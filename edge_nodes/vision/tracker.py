from config import TrackerSource, load_config
from pose_tracker import PoseTracker, SimulatedTracker, UdpSink


def main():
    cfg = load_config()

    sink = UdpSink(cfg.network.relay_ip, cfg.network.relay_port)

    print(f"Starting Optical Tracker on UDP port {cfg.network.relay_port}...")

    if cfg.tracker.source == TrackerSource.SIMULATED:
        tracker = SimulatedTracker(cfg.tracker)
        tracker.run(sink)  # No camera needed in simulated mode.
        return

    import cv2

    tracker = PoseTracker(cfg.tracker)
    camera = cv2.VideoCapture(0)  # Change index if using an external USB cam

    try:
        tracker.run(camera, sink)
    finally:
        camera.release()


if __name__ == "__main__":
    main()
