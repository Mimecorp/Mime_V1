import cv2

from config import load_config
from pose_tracker import PoseTracker, UdpSink


def main():
    cfg = load_config()

    tracker = PoseTracker(cfg.tracker)
    sink = UdpSink(cfg.network.relay_ip, cfg.network.relay_port)
    camera = cv2.VideoCapture(0)  # Change index if using an external USB cam

    print(f"Starting Optical Tracker on UDP port {cfg.network.relay_port}...")

    try:
        tracker.run(camera, sink)
    finally:
        camera.release()


if __name__ == "__main__":
    main()
