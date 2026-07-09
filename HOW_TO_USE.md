# Project Mime: User Guide

This document outlines how to boot up the entire Mime V1 architecture from a cold start. Because Mime uses a modern gRPC microservice architecture, the components can be started in separate terminals.

## Prerequisites
Ensure the ESP32 glove is powered on, connected to your local Wi-Fi, and successfully broadcasting UDP packets to your machine's IP address on Port `12345`.

---

## The Boot Sequence

### Step 1: The Central Engine (C++)
The Central Engine houses the Extended Kalman Filter and gRPC server. It must be started first.
```bash
cd ~/Mime/central_engine
bazel run //engine:server
```

### Step 2: The Relay Node (C++)
The Relay Node catches the UDP packets from the ESP32 glove and forwards them into the gRPC pipeline.
```bash
cd ~/Mime/edge_nodes
./build/mime_edge_node
```

### Step 3: The Optical Tracker (Python)
The tracker turns on your webcam, uses MediaPipe to find your right wrist, and kills the IMU drift.
```bash
cd ~/Mime/edge_nodes/vision
python3 tracker.py
```
*(Ensure your webcam is unobstructed and can clearly see your upper body/arm).*

### Step 4: The Node.js WebSockets Bridge
The Web UI requires WebSockets, so we boot a tiny Node server to bridge the gRPC stream to the browser.
```bash
cd ~/Mime/visualizer/server
node index.js
```

### Step 5: The React Digital Twin
Boot the 3D visualizer to see your arm movements in real-time.
```bash
cd ~/Mime/visualizer
npm run dev
```
Open `http://localhost:5173` in your browser.

---

## Extended Features

### Running the Inverse Kinematics (IK) Engine
If you want to calculate the physical motor joint angles for a real robot (default is the KUKA iiwa), open a new terminal:
```bash
cd ~/Mime/central_engine/ik_solver
python3 ik_engine.py
```

### Training the AI (Imitation Learning)
Once you have generated enough CSV logs using the glove, you can train the LSTM neural network.
```bash
cd ~/Mime/machine_learning
python3 train.py
```
To watch the AI generate autonomous movements after training:
```bash
python3 predict.py
```
