import cv2
import mediapipe as mp
import socket
import struct
import time

# Initialize MediaPipe Pose
mp_pose = mp.solutions.pose
pose = mp_pose.Pose(min_detection_confidence=0.5, min_tracking_confidence=0.5)

# Setup UDP Socket to send data to C++ Relay Node
UDP_IP = "127.0.0.1"
UDP_PORT = 10001
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Open Webcam (Change index if using an external USB cam)
cap = cv2.VideoCapture(0)

print(f"Starting Optical Tracker on UDP port {UDP_PORT}...")

while cap.isOpened():
    success, image = cap.read()
    if not success:
        print("Ignoring empty camera frame.")
        continue

    # To improve performance, optionally mark the image as not writeable to pass by reference.
    image.flags.writeable = False
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    results = pose.process(image)

    if results.pose_landmarks:
        # Get Right Wrist landmark (Index 16 in MediaPipe)
        # MediaPipe returns normalized coordinates (0.0 to 1.0) for x, y, and z.
        # We need to map this to an approximate metric scale (meters) for the EKF.
        wrist = results.pose_landmarks.landmark[mp_pose.PoseLandmark.RIGHT_WRIST]
        
        # Simple heuristic mapping for prototype:
        # Assuming the camera is ~0.5m away, we can scale the normalized coordinates.
        # This will need to be calibrated perfectly for a production system.
        px = (wrist.x - 0.5) * 1.0  # Center X
        py = -(wrist.y - 0.5) * 1.0 # Invert Y so up is positive
        pz = -wrist.z * 1.0         # Depth

        # Pack into 3 floats (12 bytes)
        payload = struct.pack('<fff', px, py, pz)
        
        # Blast it to the Relay Node!
        sock.sendto(payload, (UDP_IP, UDP_PORT))

    # Sleep to lock at roughly 30 FPS to save CPU
    time.sleep(1.0 / 30.0)

cap.release()
