# WSL Compatibility Tools

When developing on Windows with WSL, you might encounter an issue where inbound UDP packets from the ESP32 cannot reach the WSL `Relay Node`. This happens because Windows NAT blocks/hides the WSL virtual machine from the physical Wi-Fi network.

## How to use the UDP Bridge

If you are running the Central Engine and Relay Node inside WSL, you must run this script natively on Windows to bridge the connection:

1. Open a **Windows Command Prompt** or **PowerShell** (not WSL).
2. Run `python bridge.py`.
3. The script will listen on `0.0.0.0:8888` (catching packets hitting Windows) and forward them to your WSL IP address.

### Troubleshooting
- **Windows Firewall**: If the script is silent, Windows Defender might be blocking inbound UDP traffic on port 8888. Run this in PowerShell (as Admin) to allow it:
  `New-NetFirewallRule -DisplayName "Mime Tracker UDP" -Direction Inbound -LocalPort 8888 -Protocol UDP -Action Allow`
- **WSL IP Changed**: If you reboot your PC, your WSL IP address might change. Run `ip addr` inside WSL, and update the `WSL_IP` variable inside `bridge.py` to match.

## Native Linux Users
If you are running on a native Linux machine (Ubuntu, Raspberry Pi), **you do not need any of these tools**. The Relay Node will bind directly to your physical network interface and receive UDP packets seamlessly without firewall or NAT issues.
