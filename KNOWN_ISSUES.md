# Project Mime: Known Issues & Limitations (V1)

While the V1 Architecture of Project Mime successfully solves the core tracking problems at a fraction of the cost of enterprise systems, it relies on several prototype-level assumptions. 

If this system is to be deployed in a production, industrial, or enterprise environment, the following critical issues must be addressed in V2.

---

## 1. The Kinematic Singularity Problem (Critical)
**Description:** 
A human arm has different joint geometries and lengths than a robotic arm. Currently, the operator can freely move their human arm in any direction. However, the IK Engine (PyBullet) is mathematically constrained by the physical limits of the URDF robot it is controlling. 

If the human moves their hand into a position that the physical robot cannot mechanically reach, or if the joints align in a way that causes a mathematical "Singularity" (e.g., Gimbal Lock), the IK Engine will fail to find a valid solution. This causes the physical robot arm to violently jerk, spin out of control, or completely freeze.

**V2 Solution:** 
Implement "Workspace Constraining" or a physical Puppeteer Rig. If staying wireless, the software must implement intelligent IK fallbacks (e.g., damped least squares) that gracefully stop the robot at the edge of its physical reach, ignoring the human's out-of-bounds input until they return to a valid workspace.

---

## 2. Optical Occlusion & Lighting Failures
**Description:** 
The zero-drift capability relies entirely on a standard webcam and the MediaPipe ML model. If the room is poorly lit, if the camera experiences motion blur from fast movements, or if the operator turns their back to the camera (Occlusion), the camera loses tracking. The system will gracefully fall back to the IMU, but drift will immediately begin accumulating until the wrist is visible again.

**V2 Solution:** 
Transition to a multi-camera setup or utilize physical IR markers (like the Valve Lighthouse system) to guarantee absolute tracking regardless of human orientation.

---

## 3. Lack of Obstacle Avoidance & Path Planning
**Description:** 
Mime currently utilizes direct, real-time spatial mapping. It has no awareness of the physical environment the robot is operating in. If the human moves their hand through a physical table, the software will blindly command the robotic arm to move through the table, resulting in a physical crash and potential hardware damage.

**V2 Solution:** 
Integrate a path-planning framework (like ROS MoveIt! or an advanced PyBullet environment) that maintains a collision mesh of the physical room, preventing the IK engine from outputting joint angles that intersect with known obstacles.

---

## 4. Limited Tracking Volume
**Description:** 
Mime currently tracks exactly one 6-DOF joint (the wrist) and two flex sensors. While sufficient for basic gripper control, complex teleoperation (e.g., controlling a humanoid robot) requires tracking the elbow, bicep, shoulder, and full torso.

**V2 Solution:** 
Expand the ESP32 hardware to a multi-node IMU network (daisy-chained via I2C or wireless mesh) and upgrade the C++ Extended Kalman Filter to process a full multi-body kinematic chain.
