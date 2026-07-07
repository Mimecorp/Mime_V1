# Future Ideas & Architectural Pivots

This document tracks long-term product vision, naming concepts, and hardware pivots to transition the telepresence platform from a V1 prototype into an enterprise-grade commercial product.

## 1. Naming & Branding Concepts
*Moving away from "Project Mime" towards modern, tech-focused branding.*

*   **Vessel**: Emphasizes that the robotic arm is simply an empty vessel for human movement.
*   **NerveLink / Synapse**: Focuses on the low-latency connection between the human nervous system and the machine.
*   **Koa (or Koa Kinematics)**: "Koa" is a Hawaiian word meaning brave, bold, or warrior. It is short, punchy, and highly memorable for a Silicon Valley startup.
*   **Kinetix**: Focuses purely on the kinematics and physics of the system.

## 2. Solving the "Physics Problem" (IMU Drift)
**The Problem**: Our V1 architecture uses an ESP32 with an MPU9250 IMU. While IMUs are phenomenal at capturing high-frequency rotational changes (quaternions), they are physically incapable of tracking absolute spatial translation `[X, Y, Z]` over long periods. Double-integrating linear acceleration causes "drift", meaning the arm's position will slowly slide away from reality within seconds.

To make this commercially viable, we must provide an **absolute reference frame** to feed into the Extended Kalman Filter (EKF) to instantly correct the drift.

### Solution A: Shoulder-Mounted Camera (Computer Vision)
*The most accessible short-term solution.*
*   **How it works**: A webcam mounts on the user's shoulder looking down at the arm. The Linux PC runs a lightweight pose-estimation model (like MediaPipe) to track the hand.
*   **The Fusion**: The camera provides slow (30 FPS) but absolutely true `[X, Y, Z]` coordinates. The ESP32 provides blazing fast (100Hz+) relative motion. The EKF fuses them perfectly.
*   **Pros**: Cheap, easy to implement in software, uses existing webcams.
*   **Cons**: Line-of-sight occlusion (if the arm moves out of frame or the body blocks it, tracking breaks).

### Solution B: Lighthouse Tracking (The Holy Grail)
*The enterprise-grade solution (used in HTC Vive / Valve Index).*
*   **How it works**: We mount 3-4 tiny, 5-cent infrared photodiodes onto the ESP32 glove. We place a small "Lighthouse Base Station" on the desk. The base station emits a flash of IR light, followed by a sweeping laser.
*   **The Math**: The ESP32 acts as a stopwatch. It measures the exact microsecond difference between the flash and the laser sweep hitting the glove. Using simple trigonometry, the ESP32 calculates its exact angle relative to the base station, instantly deriving its absolute `[X, Y, Z]` position.
*   **Why it beats LIDAR/Cameras**: Unlike LIDAR, the laser does not bounce back (it's a one-way trip). Unlike cameras, there is zero heavy Computer Vision (SLAM) processing required. The ESP32 handles the math effortlessly.
*   **Pros**: Sub-millimeter accuracy, incredibly fast, zero CPU bottleneck, completely cures IMU drift.
*   **Cons**: Requires the user to have a base station active in the room.

## 3. Productization (The Desktop App)
When packaging for consumers, the terminal commands will be removed. The React WebGL visualizer will be wrapped in **Tauri** or **Electron**. Upon double-clicking the app, the UI will launch while automatically spawning the C++ Central Engine, the Node.js Bridge, and the Relay Node as invisible, lightweight background child processes. The user will experience a seamless, "single-click" application.
