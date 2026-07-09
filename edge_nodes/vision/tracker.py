import cv2
import socket
import struct
import time
import math

# Setup UDP Socket to send data to C++ Relay Node
UDP_IP = "127.0.0.1"
UDP_PORT = 10001
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"Starting Optical Tracker on UDP port {UDP_PORT}... (Simulated Mode)")

t = 0.0
while True:
    # Simulate a gentle hand movement for the demo
    px = math.sin(t) * 0.2
    py = math.cos(t) * 0.2 + 0.3
    pz = math.sin(t * 0.5) * 0.1 - 0.5

    # Pack into 3 floats (12 bytes)
    payload = struct.pack('<fff', px, py, pz)
    
    # Blast it to the Relay Node!
    sock.sendto(payload, (UDP_IP, UDP_PORT))

    t += 0.05
    time.sleep(1.0 / 30.0)
