import yaml
from enum import Enum
from pydantic import BaseModel, Field


class PoseLandmark(str, Enum):
    """Supported MediaPipe Pose landmarks. Value == name so the YAML stays
    human-readable and Pydantic rejects anything that isn't listed here."""
    LEFT_WRIST = "LEFT_WRIST"
    RIGHT_WRIST = "RIGHT_WRIST"


class TrackerSource(str, Enum):
    """Where wrist coordinates come from: a real camera + MediaPipe, or the
    camera-free simulated motion for demos."""
    CAMERA = "CAMERA"
    SIMULATED = "SIMULATED"


class ScaleConfig(BaseModel):
    x: float
    y: float
    z: float

class NetworkConfig(BaseModel):
    relay_ip: str
    relay_port: int = Field(ge=1, le=65535)

class TrackerConfig(BaseModel):
    source: TrackerSource = TrackerSource.SIMULATED
    detection_confidence: float = Field(ge=0.0, le=1.0)
    tracking_confidence: float = Field(ge=0.0, le=1.0)
    target_fps: int = Field(gt=0)
    landmark: PoseLandmark
    scale : ScaleConfig

class AppConfig(BaseModel):
    tracker: TrackerConfig
    network: NetworkConfig

def load_config(path="config.yaml"):
    with open(path, 'r') as f:
        data = yaml.safe_load(f)

    return AppConfig(**data)

