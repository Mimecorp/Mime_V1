#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <MPU9250.h>
#include "../../shared/udp_packet.h"

// TODO: Replace with your actual Wi-Fi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// TODO: Replace with the IP address of the Linux PC running the Relay
const char* relay_ip = "192.168.1.100"; 
const int relay_port = 8888;

WiFiUDP udp;
mime::edge::UdpTelemetryPacket packet;

// Hardware Definitions
MPU9250 mpu;
const int PIN_FLEX_THUMB = 34; // Ensure flex sensors use a voltage divider!
const int PIN_FLEX_INDEX = 35;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
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
        while (1) {
            Serial.println("MPU connection failed. Please check your wiring.");
            delay(5000);
        }
    }

    // Optional: calibrate on startup. Make sure the IMU is flat and still!
    Serial.println("Calibrating IMU... Keep it flat and still.");
    mpu.calibrateAccelGyro();
    mpu.calibrateMag();
    Serial.println("Calibration complete!");
}

void loop() {
    // Read the MPU9250. This library automatically runs the Mahony filter
    // in the background to fuse Accel/Gyro/Mag into a clean quaternion.
    if (mpu.update()) {
        packet.timestamp_us = micros();
        
        // Extract the calculated quaternions
        packet.q_w = mpu.getQuaternionW();
        packet.q_x = mpu.getQuaternionX();
        packet.q_y = mpu.getQuaternionY();
        packet.q_z = mpu.getQuaternionZ();
        
        // Extract raw acceleration (in Gs, typically converted to m/s^2 later, but we'll send raw Gs for now, or assume it's in g)
        packet.accel_x = mpu.getAccX();
        packet.accel_y = mpu.getAccY();
        packet.accel_z = mpu.getAccZ();
        
        // Read actual flex sensors using ESP32's built-in ADC
        int raw_thumb = analogRead(PIN_FLEX_THUMB);
        int raw_index = analogRead(PIN_FLEX_INDEX);
        
        // Normalize to 0.0 - 1.0
        packet.flex_thumb = raw_thumb / 4095.0f;
        packet.flex_index = raw_index / 4095.0f;
        
        // Mock optical data for now
        packet.camera_z = 1.0;
        
        // Send UDP packet to Linux Relay
        udp.beginPacket(relay_ip, relay_port);
        udp.write((uint8_t*)&packet, sizeof(packet));
        udp.endPacket();
    }
}
