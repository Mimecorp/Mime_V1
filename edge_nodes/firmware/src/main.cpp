#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <MPU9250.h>
#include <MadgwickAHRS.h>
#include "../shared/udp_packet.h"
#include <config.h>

WiFiUDP udp;
mime::edge::UdpTelemetryPacket packet;

// Hardware Definitions
MPU9250 mpu;
Madgwick filter;
const int PIN_FLEX_THUMB = 34; // Ensure flex sensors use a voltage divider!
const int PIN_FLEX_INDEX = 35;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    // Connect to Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());

    // Setup ADC resolution (12-bit by default: 0-4095)
    analogReadResolution(12);

    // Initialize MPU9250
    Serial.println("Initializing MPU9250...");
    if (!mpu.setup(0x68)) {  // change to 0x69 if your AD0 pin is high
        Serial.println("Warning: MPU9250 setup returned false. Bypassing magnetometer and proceeding...");
    }

    // Calibrate on startup. Make sure the IMU is flat and still!
    Serial.println("Calibrating IMU... Keep it flat and still.");
    mpu.calibrateAccelGyro();
    // mpu.calibrateMag(); // Removed to bypass missing magnetometer
    
    // Initialize 6-DOF Madgwick filter at 100Hz
    filter.begin(100.0f);
    
    Serial.println("Calibration complete!");
}

void loop() {
    // Read raw data from the MPU9250
    if (mpu.update()) {
        packet.timestamp_us = micros();
        
        // Extract raw acceleration (in Gs)
        packet.accel_x = mpu.getAccX();
        packet.accel_y = mpu.getAccY();
        packet.accel_z = mpu.getAccZ();
        
        // Feed raw Gyro and Accel into Madgwick 6-DOF filter (ignoring magnetometer)
        filter.updateIMU(mpu.getGyroX(), mpu.getGyroY(), mpu.getGyroZ(), 
                         packet.accel_x, packet.accel_y, packet.accel_z);
        
        // Extract the calculated quaternions from the Madgwick filter
        packet.q_w = filter.q0;
        packet.q_x = filter.q1;
        packet.q_y = filter.q2;
        packet.q_z = filter.q3;
        
        // Read actual flex sensors using ESP32's built-in ADC
        int raw_thumb = analogRead(PIN_FLEX_THUMB);
        int raw_index = analogRead(PIN_FLEX_INDEX);
        
        // Normalize to 0.0 - 1.0
        packet.flex_thumb = raw_thumb / 4095.0f;
        packet.flex_index = raw_index / 4095.0f;
        
        // Mock optical data for now
        packet.camera_z = 1.0;
        
        // Send UDP packet to Linux Relay
        udp.beginPacket(RELAY_IP, RELAY_PORT);
        udp.write((uint8_t*)&packet, sizeof(packet));
        udp.endPacket();
    }
}
