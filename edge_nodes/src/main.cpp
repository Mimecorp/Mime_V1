#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include "telemetry.grpc.pb.h"
#include "../shared/udp_packet.h"

struct CameraData {
    float x;
    float y;
    float z;
    bool has_data;
};

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using mime::telemetry::MimeTelemetryService;
using mime::telemetry::EdgeTelemetry;
using mime::telemetry::StreamResponse;

class RelayClient {
public:
    RelayClient(std::shared_ptr<Channel> channel)
        : stub_(MimeTelemetryService::NewStub(channel)) {
        latest_camera_data_.has_data = false;
    }

    void RunCameraListener() {
        int cam_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (cam_socket < 0) return;
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(10001);
        
        if (bind(cam_socket, (const sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind camera UDP socket." << std::endl;
            return;
        }
        
        std::cout << "Relay Node listening for Optical Tracker on port 10001..." << std::endl;
        
        float buffer[3];
        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            ssize_t bytes = recvfrom(cam_socket, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, &client_len);
            if (bytes == 12) {
                std::lock_guard<std::mutex> lock(camera_mutex_);
                latest_camera_data_.x = buffer[0];
                latest_camera_data_.y = buffer[1];
                latest_camera_data_.z = buffer[2];
                latest_camera_data_.has_data = true;
            }
        }
    }

    void RunRelay() {
        // Spawn the optical tracker background thread
        std::thread cam_thread(&RelayClient::RunCameraListener, this);
        cam_thread.detach();
        // 1. Setup UDP Socket to listen for ESP32 broadcasts
        int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket < 0) {
            std::cerr << "Failed to create UDP socket." << std::endl;
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(8888);

        if (bind(udp_socket, (const sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind UDP socket to port 8888." << std::endl;
            close(udp_socket);
            return;
        }

        std::cout << "Relay Node listening for ESP32 UDP packets on port 8888..." << std::endl;

        // 2. Setup gRPC Stream to Central Engine
        ClientContext context;
        StreamResponse response;
        std::unique_ptr<grpc::ClientWriter<EdgeTelemetry>> writer(
            stub_->StreamTelemetry(&context, &response));

        std::cout << "Relay Node connected to Central Engine gRPC stream." << std::endl;

        mime::edge::UdpTelemetryPacket packet;
        int packet_count = 0;
        
        // 3. Main ingest loop: UDP -> gRPC
        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            // Blocking call to receive fast UDP packets
            ssize_t bytes_received = recvfrom(udp_socket, &packet, sizeof(packet), 0,
                                              (sockaddr*)&client_addr, &client_len);
                                              
            if (bytes_received == sizeof(packet)) {
                EdgeTelemetry telemetry;
                telemetry.set_timestamp_ns(packet.timestamp_us * 1000ULL);
                
                auto* imu = telemetry.add_imu_array();
                imu->set_sensor_id("wrist_imu_01");
                auto* q = imu->mutable_orientation();
                q->set_w(packet.q_w); q->set_x(packet.q_x); q->set_y(packet.q_y); q->set_z(packet.q_z);
                
                auto* accel = imu->mutable_linear_acceleration();
                accel->set_x(packet.accel_x);
                accel->set_y(packet.accel_y);
                accel->set_z(packet.accel_z);
                
                auto* flex = telemetry.mutable_flex_sensors();
                flex->set_thumb(packet.flex_thumb);
                flex->set_index(packet.flex_index);
                
                // Inject the latest optical tracking position if the Python daemon sent it
                bool include_cam = false;
                CameraData cam_data;
                {
                    std::lock_guard<std::mutex> lock(camera_mutex_);
                    if (latest_camera_data_.has_data) {
                        cam_data = latest_camera_data_;
                        include_cam = true;
                        latest_camera_data_.has_data = false; // consume it so we don't send stale data repeatedly
                    }
                }
                
                if (include_cam) {
                    auto* pos = telemetry.mutable_optical_tracking_position();
                    pos->set_x(cam_data.x);
                    pos->set_y(cam_data.y);
                    pos->set_z(cam_data.z);
                }
                
                // Blast to Central Engine!
                if (!writer->Write(telemetry)) {
                    std::cerr << "gRPC stream lost." << std::endl;
                    break;
                }
                
                packet_count++;
                if (packet_count % 100 == 0) {
                    std::cout << "Relayed 100 packets to Central Engine..." << std::endl;
                }
            }
        }

        writer->WritesDone();
        writer->Finish();
        close(udp_socket);
    }

private:
    std::unique_ptr<MimeTelemetryService::Stub> stub_;
    std::mutex camera_mutex_;
    CameraData latest_camera_data_;
};

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    RelayClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    client.RunRelay();
    return 0;
}
