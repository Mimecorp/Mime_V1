import socket

# Catch packets hitting Windows from the ESP32
WINDOWS_IP = "0.0.0.0"
WINDOWS_PORT = 8888

# Throw them over the fence to your WSL IP address
# UPDATE THIS IF YOUR WSL IP CHANGES
WSL_IP = "172.23.183.10"
WSL_PORT = 8888

sock_in = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_in.bind((WINDOWS_IP, WINDOWS_PORT))
sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"WSL UDP Bridge listening on port {WINDOWS_PORT} and forwarding to WSL at {WSL_IP}:{WSL_PORT}...")

packet_count = 0
while True:
    data, addr = sock_in.recvfrom(1024)
    packet_count += 1
    if packet_count % 100 == 0:
        print(f"Received 100 packets from ESP32 at {addr}! Forwarding to WSL...")
    sock_out.sendto(data, (WSL_IP, WSL_PORT))
