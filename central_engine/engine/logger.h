#pragma once
#include <fstream>
#include <string>

namespace mime {
namespace engine {

class DataLogger {
public:
    DataLogger();
    ~DataLogger();
    
    // Open a new file (e.g. with a timestamp)
    bool Open(const std::string& filename);
    
    // Close current file
    void Close();

    // Log a single row of telemetry
    void LogState(uint64_t timestamp_ns, 
                  double px, double py, double pz, 
                  double vx, double vy, double vz, 
                  double qw, double qx, double qy, double qz, 
                  double flex_thumb, double flex_index);

private:
    std::ofstream file_;
    bool is_open_;
};

} // namespace engine
} // namespace mime
