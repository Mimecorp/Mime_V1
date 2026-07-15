import math
import struct
import time
import socket
from dataclasses import dataclass
from config import TrackerConfig


@dataclass
class WristState:
    px : float
    py : float
    pz : float

class UdpSink:
    def __init__(self, ip: str, port: int):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.ip = ip
        self.port = port

    def send(self, state: WristState):
        payload = struct.pack('<fff', state.px, state.py, state.pz)
        self.sock.sendto(payload, (self.ip, self.port))

class PoseTracker:
    def __init__(self, config: TrackerConfig):
        import mediapipe as mp # Lazy import -> only needed when we have the camera stack

        self.config = config

        mp_pose = mp.solutions.pose
        self.pose = mp_pose.Pose(
            min_detection_confidence=config.detection_confidence,
            min_tracking_confidence=config.tracking_confidence,
        )

        # Turn the landmark name from config ("RIGHT_WRIST") into the enum member.
        self.landmark = getattr(mp_pose.PoseLandmark, config.landmark.value)

    def process_frame(self, image) -> WristState | None:
        import cv2 # Lazy import -> only needed when we have the camera stack

        # Pure: run pose estimation and return the scaled wrist coordinate, or None.
        image.flags.writeable = False
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        results = self.pose.process(image)

        if not results.pose_landmarks:
            return None

        wrist = results.pose_landmarks.landmark[self.landmark]

        px = (wrist.x - 0.5) * self.config.scale.x   # Center X
        py = -(wrist.y - 0.5) * self.config.scale.y  # Invert Y so up is positive
        pz = -wrist.z * self.config.scale.z          # Depth

        return WristState(px, py, pz)

    def run(self, camera, sink):
        while camera.isOpened():
            success, image = camera.read()
            if not success:
                print("Ignoring empty camera frame.")
                continue

            state = self.process_frame(image)
            if state is not None:
                sink.send(state)

            # Throttle to the configured frame rate to save CPU.
            time.sleep(1.0 / self.config.target_fps)

class SimulatedTracker:
    
    """Camera-free source: emits a canned sine/cosine wrist motion so the
    pipeline runs without a webcam. Drop-in for PoseTracker as far as the sink
    is concerned since it produces the same WristState."""

    def __init__(self, config: TrackerConfig):
        self.config = config
        self.t = 0.0

    def next_state(self) -> WristState:
        # Pure: advance the clock and return the simulated wrist coordinate.
        px = math.sin(self.t) * 0.2
        py = math.cos(self.t) * 0.2 + 0.3
        pz = math.sin(self.t * 0.5) * 0.1 - 0.5

        self.t += 0.05
        return WristState(px, py, pz)

    def run(self, sink):
        print("Running in Simulated Mode.")
        while True:
            sink.send(self.next_state())

            # Throttle to the configured frame rate to save CPU.
            time.sleep(1.0 / self.config.target_fps)
