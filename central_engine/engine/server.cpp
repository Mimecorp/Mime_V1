#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <ctime>
#include "proto/telemetry.grpc.pb.h"
#include "ekf.h"
#include "logger.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using mime::telemetry::MimeTelemetryService;
using mime::telemetry::EdgeTelemetry;
using mime::telemetry::StreamResponse;
using mime::telemetry::FusedState;
using mime::telemetry::SubscribeRequest;

// Global state for pub/sub
std::mutex g_state_mutex;
std::condition_variable g_state_cv;
FusedState g_latest_state;
bool g_has_new_state = false;

// This service implementation runs in the Central Engine
class MimeTelemetryServiceImpl final : public MimeTelemetryService::Service {
public:
    MimeTelemetryServiceImpl() {}

    Status StreamTelemetry(ServerContext* context, 
                           ServerReader<EdgeTelemetry>* reader, 
                           StreamResponse* response) override {
        EdgeTelemetry telemetry;
        int packet_count = 0;
        uint64_t last_timestamp = 0;
        
        std::cout << "[Server] Edge Node connected. Stream opened." << std::endl;

        // Initialize Data Logger
        mime::engine::DataLogger logger;
        std::time_t t = std::time(nullptr);
        char filename[64];
        std::strftime(filename, sizeof(filename), "telemetry_log_%Y%m%d_%H%M%S.csv", std::localtime(&t));
        logger.Open(filename);
        std::cout << "[Server] Logging telemetry to " << filename << std::endl;

        // Initialize EKF
        mime::engine::EkfFilter ekf;

        // This loop reads from the binary stream as fast as the edge node sends it
        while (reader->Read(&telemetry)) {
            packet_count++;
            
            // Calculate delta time
            double dt = 0.01; // Default to 10ms
            if (last_timestamp != 0) {
                dt = (telemetry.timestamp_ns() - last_timestamp) / 1e9;
            }
            last_timestamp = telemetry.timestamp_ns();
            
            // Extract Quaternions and Acceleration
            double q_w = 1.0, q_x = 0.0, q_y = 0.0, q_z = 0.0;
            double a_x = 0.0, a_y = 0.0, a_z = 0.0;
            if (telemetry.imu_array_size() > 0) {
                const auto& imu = telemetry.imu_array(0);
                if (imu.has_orientation()) {
                    q_w = imu.orientation().w();
                    q_x = imu.orientation().x();
                    q_y = imu.orientation().y();
                    q_z = imu.orientation().z();
                }
                if (imu.has_linear_acceleration()) {
                    a_x = imu.linear_acceleration().x();
                    a_y = imu.linear_acceleration().y();
                    a_z = imu.linear_acceleration().z();
                }
            }

            // EKF Predict Step (High-frequency IMU)
            ekf.predict(dt, q_w, q_x, q_y, q_z, a_x, a_y, a_z);

            // EKF Update Step (Low-frequency camera/optical tracking)
            if (telemetry.has_optical_tracking_position()) {
                const auto& pos = telemetry.optical_tracking_position();
                ekf.update(pos.x(), pos.y(), pos.z());
            }

            Eigen::VectorXd state = ekf.getState();

            // Extract Flex Sensors for logging
            double flex_thumb = 0.0, flex_index = 0.0;
            if (telemetry.has_flex_sensors()) {
                flex_thumb = telemetry.flex_sensors().thumb();
                flex_index = telemetry.flex_sensors().index();
            }

            // Log Data to Disk
            logger.LogState(telemetry.timestamp_ns(), 
                            state(0), state(1), state(2), // position
                            state(3), state(4), state(5), // velocity
                            q_w, q_x, q_y, q_z,           // orientation
                            flex_thumb, flex_index);      // flex

            // Notify subscribers
            {
                std::lock_guard<std::mutex> lock(g_state_mutex);
                g_latest_state.set_px(state(0));
                g_latest_state.set_py(state(1));
                g_latest_state.set_pz(state(2));
                g_latest_state.set_vx(state(3));
                g_latest_state.set_vy(state(4));
                g_latest_state.set_vz(state(5));
                
                auto* q = g_latest_state.mutable_orientation();
                q->set_w(q_w); q->set_x(q_x); q->set_y(q_y); q->set_z(q_z);
                
                g_has_new_state = true;
            }
            g_state_cv.notify_all();

            // Print out every 100th packet (1 second of data at 100Hz) to verify
            if (packet_count % 100 == 0) { 
                std::cout << "[Server/EKF] 100 Packets Processed. "
                          << "Fused Position (X,Y,Z): [" << state(0) << ", " << state(1) << ", " << state(2) << "] m, "
                          << "Fused Velocity (X,Y,Z): [" << state(3) << ", " << state(4) << ", " << state(5) << "] m/s"
                          << std::endl;
            }
        }
        
        logger.Close();
        std::cout << "[Server] Edge Node disconnected." << std::endl;
        response->set_success(true);
        return Status::OK;
    }

    Status SubscribeFusedState(ServerContext* context, 
                               const SubscribeRequest* request, 
                               ServerWriter<FusedState>* writer) override {
        std::cout << "[Server] Visualizer client subscribed." << std::endl;
        
        while (!context->IsCancelled()) {
            FusedState state_copy;
            {
                std::unique_lock<std::mutex> lock(g_state_mutex);
                // Wait for new state or cancellation
                g_state_cv.wait(lock, [context] { return g_has_new_state || context->IsCancelled(); });
                
                if (context->IsCancelled()) break;
                
                state_copy = g_latest_state;
                g_has_new_state = false;
            }
            
            if (!writer->Write(state_copy)) {
                break;
            }
        }
        
        std::cout << "[Server] Visualizer client disconnected." << std::endl;
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    MimeTelemetryServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Mime Central Engine Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
