import grpc
import rerun as rr
import time
import math

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '../ik_solver'))
import telemetry_pb2
import telemetry_pb2_grpc

def main():
    print("[Rerun Logger] Connecting to Mime Central Engine...")
    
    channel = grpc.insecure_channel('localhost:50051')
    stub = telemetry_pb2_grpc.MimeTelemetryServiceStub(channel)
    
    # Initialize Rerun Live Viewer
    rr.init("Mime_Telemetry_Tracker", spawn=True)
    
    try:
        # Subscribe to the fused Cartesian state
        stream = stub.SubscribeFusedState(telemetry_pb2.SubscribeRequest())
        
        print("[Rerun Logger] Connected! Streaming 3D data to Rerun Viewer...")
        
        for state in stream:
            # Rerun uses X, Y, Z for positions
            position = [state.px, state.py, state.pz]
            
            # Rerun uses X, Y, Z, W for quaternions
            quaternion = [state.orientation.x, state.orientation.y, state.orientation.z, state.orientation.w]
            
            # Log the Hand's 3D Transform
            rr.log(
                "Mime/Hand",
                rr.Transform3D(
                    translation=position,
                    rotation=rr.Quaternion(xyzw=quaternion)
                )
            )
            
            # Log the Flex Sensors as a time-series scalar graph
            rr.log("Mime/Flex/Thumb", rr.Scalar(state.flex_sensors.thumb))
            rr.log("Mime/Flex/Index", rr.Scalar(state.flex_sensors.index))
            
            # (Optional) Log velocity vectors as 3D arrows
            rr.log(
                "Mime/Hand/Velocity", 
                rr.Arrows3D(
                    vectors=[[state.vx, state.vy, state.vz]],
                    colors=[[255, 0, 0]] # Red velocity vector
                )
            )
            
    except grpc.RpcError as e:
        print(f"[Rerun Logger] gRPC Error: {e.details()}")

if __name__ == '__main__':
    main()
