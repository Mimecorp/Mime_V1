#include <memory>
#include <thread>
#include <chrono>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include <grpcpp/grpcpp.h>
#include "telemetry.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using mime::telemetry::MimeTelemetryService;
using mime::telemetry::FusedState;
using mime::telemetry::SubscribeRequest;

class MoveItBridgeNode : public rclcpp::Node {
public:
    MoveItBridgeNode(const std::string& node_name, const rclcpp::NodeOptions& options)
        : Node(node_name, options) {
        
        // Create publisher for Servo Pose Tracking
        // The topic name should match what is configured in servo_config.yaml (e.g., ~/target_pose)
        pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/target_pose", rclcpp::SystemDefaultsQoS());
        
        // Connect to gRPC Server
        channel_ = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
        stub_ = MimeTelemetryService::NewStub(channel_);
        
        RCLCPP_INFO(this->get_logger(), "Connecting to gRPC server at localhost:50051...");
    }

    void start_grpc_thread() {
        grpc_thread_ = std::thread([this]() {
            SubscribeRequest request;
            ClientContext context;
            
            std::unique_ptr<ClientReader<FusedState>> reader(
                stub_->SubscribeFusedState(&context, request));
            
            FusedState state;
            while (reader->Read(&state)) {
                geometry_msgs::msg::PoseStamped target_pose;
                // MoveIt Servo expects stamped messages with a valid frame_id
                target_pose.header.stamp = this->get_clock()->now();
                target_pose.header.frame_id = "base_link"; // Assuming KUKA base, adjust as needed
                
                // Apply scaling to fit KUKA workspace
                target_pose.pose.position.x = state.px() * 0.5;
                target_pose.pose.position.y = state.py() * 0.5;
                target_pose.pose.position.z = state.pz() * 0.5 + 0.5;
                
                target_pose.pose.orientation.w = state.orientation().w();
                target_pose.pose.orientation.x = state.orientation().x();
                target_pose.pose.orientation.y = state.orientation().y();
                target_pose.pose.orientation.z = state.orientation().z();
                
                // Publish at high-frequency directly to Servo's PID controller!
                pose_pub_->publish(target_pose);
            }
            Status status = reader->Finish();
            if (!status.ok()) {
                RCLCPP_ERROR(this->get_logger(), "gRPC stream failed: %s", status.error_message().c_str());
            }
        });
    }

    ~MoveItBridgeNode() {
        if (grpc_thread_.joinable()) {
            grpc_thread_.join();
        }
    }

private:
    std::shared_ptr<Channel> channel_;
    std::unique_ptr<MimeTelemetryService::Stub> stub_;
    std::thread grpc_thread_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    
    rclcpp::NodeOptions node_options;
    auto node = std::make_shared<MoveItBridgeNode>("moveit_bridge_node", node_options);
    
    node->start_grpc_thread();
    
    rclcpp::spin(node);
    
    rclcpp::shutdown();
    return 0;
}
