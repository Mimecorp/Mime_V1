#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grpcpp/grpcpp.h>
#include "telemetry.grpc.pb.h"
#include "../shared/udp_packet.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using mime::telemetry::MimeTelemetryService;
using mime::telemetry::EdgeTelemetry;
using mime::telemetry::StreamResponse;

class RelayClient {
public:
    RelayClient(std::shared_ptr<Channel> channel)
        : stub_(MimeTelemetryService::NewStub(channel)) {}

    void RunRelay() {
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
                
                auto* pos = telemetry.mutable_optical_tracking_position();
                pos->set_z(packet.camera_z);
                
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
};

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    RelayClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    client.RunRelay();
    return 0;
}
