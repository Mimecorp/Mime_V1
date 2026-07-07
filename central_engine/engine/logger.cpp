#include "logger.h"
#include <iostream>

namespace mime {
namespace engine {

DataLogger::DataLogger() : is_open_(false) {}

DataLogger::~DataLogger() {
    Close();
}

bool DataLogger::Open(const std::string& filename) {
    Close(); // ensure any previous file is closed
    file_.open(filename);
    if (!file_.is_open()) {
        std::cerr << "[Logger] Failed to open file: " << filename << std::endl;
        return false;
    }
    
    // Write CSV Header
    file_ << "timestamp_ns,px,py,pz,vx,vy,vz,qw,qx,qy,qz,flex_thumb,flex_index\n";
    is_open_ = true;
    return true;
}

void DataLogger::Close() {
    if (file_.is_open()) {
        file_.close();
        is_open_ = false;
    }
}

void DataLogger::LogState(uint64_t timestamp_ns, 
                          double px, double py, double pz, 
                          double vx, double vy, double vz, 
                          double qw, double qx, double qy, double qz, 
                          double flex_thumb, double flex_index) {
    if (!is_open_) return;
    
    file_ << timestamp_ns << ","
          << px << "," << py << "," << pz << ","
          << vx << "," << vy << "," << vz << ","
          << qw << "," << qx << "," << qy << "," << qz << ","
          << flex_thumb << "," << flex_index << "\n";
}

} // namespace engine
} // namespace mime
