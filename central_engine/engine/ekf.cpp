#include "ekf.h"
#include <iostream>

namespace mime {
namespace engine {

EkfFilter::EkfFilter() {
    // 6D State: [px, py, pz, vx, vy, vz]
    x_ = Eigen::VectorXd(6);
    x_ << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
    
    P_ = Eigen::MatrixXd::Identity(6, 6);
    
    Q_ = Eigen::MatrixXd::Identity(6, 6) * 0.01; 
    
    R_ = Eigen::MatrixXd::Identity(3, 3) * 0.001;  
    
    // We only measure position (px, py, pz)
    H_ = Eigen::MatrixXd::Zero(3, 6);
    H_(0, 0) = 1.0;
    H_(1, 1) = 1.0;
    H_(2, 2) = 1.0;
    
    F_ = Eigen::MatrixXd::Identity(6, 6);
}

void EkfFilter::predict(double dt, double q_w, double q_x, double q_y, double q_z, double a_x, double a_y, double a_z) {
    // 1. Update State Transition Matrix for this timestep
    F_(0, 3) = dt;
    F_(1, 4) = dt;
    F_(2, 5) = dt;
    
    // 2. Rotate local acceleration to global frame using quaternion
    // Convert IMU acceleration (g's) to m/s^2
    Eigen::Vector3d local_a(a_x * 9.81, a_y * 9.81, a_z * 9.81);
    
    Eigen::Quaterniond q(q_w, q_x, q_y, q_z);
    Eigen::Vector3d global_a = q.toRotationMatrix() * local_a;
    
    // 3. Subtract gravity (assuming Z is up)
    global_a(2) -= 9.81;
    
    // 4. State extrapolation
    // Position += Velocity * dt + 0.5 * accel * dt^2
    x_(0) += x_(3) * dt + 0.5 * global_a(0) * dt * dt;
    x_(1) += x_(4) * dt + 0.5 * global_a(1) * dt * dt;
    x_(2) += x_(5) * dt + 0.5 * global_a(2) * dt * dt;
    
    // Velocity += accel * dt
    x_(3) += global_a(0) * dt;
    x_(4) += global_a(1) * dt;
    x_(5) += global_a(2) * dt;
    
    // 5. Covariance extrapolation
    P_ = F_ * P_ * F_.transpose() + Q_;
}

void EkfFilter::update(double p_x, double p_y, double p_z) {
    Eigen::Vector3d z(p_x, p_y, p_z);
    
    Eigen::VectorXd y = z - H_ * x_;
    Eigen::MatrixXd S = H_ * P_ * H_.transpose() + R_;
    Eigen::MatrixXd K = P_ * H_.transpose() * S.inverse();
    
    x_ = x_ + K * y;
    P_ = (Eigen::MatrixXd::Identity(6, 6) - K * H_) * P_;
}

Eigen::VectorXd EkfFilter::getState() const {
    return x_;
}

} // namespace engine
} // namespace mime
