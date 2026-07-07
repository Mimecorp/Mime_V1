#pragma once
#include <Eigen/Dense>

namespace mime {
namespace engine {

class EkfFilter {
public:
    EkfFilter();
    
    // Predict step driven by high-frequency IMU
    // Takes time delta, quaternion (w, x, y, z), and linear acceleration (x, y, z)
    void predict(double dt, double q_w, double q_x, double q_y, double q_z, double a_x, double a_y, double a_z);
    
    // Update step driven by low-frequency optical tracker (gives absolute 3D position)
    void update(double p_x, double p_y, double p_z);

    // Get current state [px, py, pz, vx, vy, vz]
    Eigen::VectorXd getState() const;

private:
    // State vector (6D)
    Eigen::VectorXd x_; 
    
    // Covariance Matrix (6x6)
    Eigen::MatrixXd P_; 
    
    // Process Noise (6x6)
    Eigen::MatrixXd Q_;
    
    // Measurement Noise (3x3)
    Eigen::MatrixXd R_;
    
    // Measurement Matrix (3x6)
    Eigen::MatrixXd H_;
    
    // State Transition Matrix (6x6)
    Eigen::MatrixXd F_;
};

} // namespace engine
} // namespace mime
