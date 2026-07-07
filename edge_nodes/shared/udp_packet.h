#pragma once
#include <stdint.h>

namespace mime {
namespace edge {

// Packed to ensure consistent byte alignment across different architectures (ESP32 vs x86 Linux)
#pragma pack(push, 1)
struct UdpTelemetryPacket {
    uint64_t timestamp_us;
    
    // BNO085/MPU9250 Quaternion (IMU)
    float q_w;
    float q_x;
    float q_y;
    float q_z;

    // Linear Acceleration (m/s^2)
    float accel_x;
    float accel_y;
    float accel_z;

    // Flex sensors (ADC values mapped to 0.0 - 1.0)
    float flex_thumb;
    float flex_index;
    
    // Optical Tracking (Placeholder for now)
    float camera_z;
};
#pragma pack(pop)

} // namespace edge
} // namespace mime
