#pragma once

// ==========================================
// Mime Edge Node Configuration
// ==========================================

// --- Wi-Fi Credentials ---
// (This file is gitignored, so your password is safe)
const char* WIFI_SSID = "YOUR_WIFI_SSID_HERE";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE";

// --- Network Configuration ---
// If running native Linux, this might be your actual local IP (e.g. 192.168.1.50)
// If running WSL on Windows, this must be your Windows IP (e.g. 192.168.1.120)
const char* RELAY_IP = "192.168.1.120";
const int RELAY_PORT = 8888;
