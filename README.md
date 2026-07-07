# Project Mime (V1 Foundation)

Project Mime is a high-performance, real-time robotic telepresence system. It is designed to capture human kinesthetic data (using IMUs and flex sensors on a wearable glove), process it through an advanced mathematical engine, and stream the absolute physical coordinates to a visualizer or physical robotic arm with ultra-low latency.

## 🚀 Architecture

The system is broken down into three major microservices:

### 1. Edge Nodes (`/edge_nodes`)
The data ingestion layer.
*   **Hardware Firmware (`/firmware`)**: Runs on an ESP32-WROOM. It interfaces with an MPU9250 IMU to perform Mahony sensor fusion (calculating quaternions) and reads analog flex sensors. It broadcasts this raw data over Wi-Fi via UDP at 100Hz.
*   **Relay Node (`/src`)**: A native C++ Linux daemon that catches the high-speed UDP packets from the air, repackages them into structured Protobuf messages, and streams them over a persistent gRPC connection to the Central Engine.

### 2. Central Engine (`/central_engine`)
The "Brain" of the operation, built in C++ and managed with Bazel.
*   **Mathematical Processing**: Receives the raw gRPC stream and pushes the kinematics through a custom **6-DOF Extended Kalman Filter (EKF)** using the Eigen library. It mathematically strips away gravity from the raw linear acceleration vectors and integrates the remaining forces to calculate a highly stable, absolute 3D position `[px, py, pz]` and velocity.
*   **Data Logging**: Features a decoupled, high-speed C++ `DataLogger` that intercepts the output of the EKF and writes the perfect 6-DOF coordinates and flex sensor values to uniquely timestamped CSV files at 100Hz for future Machine Learning training.
*   **Publisher**: Exposes a `SubscribeFusedState` RPC, allowing visualizers to subscribe to the math engine's output in real-time.

### 3. Visualizer (`/visualizer`)
A hyper-professional 3D telemetry dashboard built with React, Vite, and Three.js (`react-three-fiber`).
*   **The Bridge (`/server`)**: Because browsers cannot natively speak gRPC, a lightweight Node.js Backend-For-Frontend (BFF) connects to the Central Engine via gRPC and instantly translates the stream into WebSockets (Socket.io).
*   **The Dashboard**: A glassmorphic web application featuring cinematic lighting, smooth quaternion interpolation (`slerp`), and native **URDF Parsing**. You can drop any industry-standard robot model into the visualizer, and it will perfectly mirror the physical movements of the wearable edge node.

## 🛠️ How to Run

Booting the full stack requires running the 4 sub-systems in parallel:

**1. Start the Central Engine (gRPC Server)**
```bash
cd central_engine
bazel run //engine:server
```

**2. Start the Node.js Bridge**
```bash
cd visualizer/server
npm install
node index.js
```

**3. Start the Web Visualizer**
```bash
cd visualizer
npm install
npm run dev
```
*(Open `http://localhost:5173` in your browser)*

**4. Start the Edge Node Relay (Data Ingestion)**
```bash
cd edge_nodes
cmake -B build
make -C build
./build/mime_edge_node
```

*(Finally, power on your ESP32 to begin streaming!)*

## 📦 Tech Stack
*   **C++20**: Central Engine, Relay Node, EKF Math
*   **Eigen**: Matrix mathematics
*   **gRPC / Protobuf**: Core communication backbone
*   **Bazel & CMake**: Build systems
*   **React + Three.js**: WebGL Visualization
*   **Node.js**: WebSocket Bridge
