import grpc
import pybullet as p
import pybullet_data
import time
import math
import telemetry_pb2
import telemetry_pb2_grpc

def main():
    print("[IK Engine] Initializing PyBullet Physics Engine...")
    
    # Start PyBullet in DIRECT mode (no GUI needed, just pure math)
    # If you want to see the robot, you can change p.DIRECT to p.GUI
    physicsClient = p.connect(p.DIRECT)
    
    # Load PyBullet's built-in URDF search path
    p.setAdditionalSearchPath(pybullet_data.getDataPath())
    
    # Load the KUKA iiwa robot arm (7-DOF industrial robot)
    robot_id = p.loadURDF("kuka_iiwa/model.urdf", [0, 0, 0], useFixedBase=True)
    num_joints = p.getNumJoints(robot_id)
    
    # The end-effector is the last link on the KUKA arm
    end_effector_index = 6 
    
    print(f"[IK Engine] Loaded KUKA iiwa with {num_joints} joints.")
    print("[IK Engine] Connecting to C++ Central Engine via gRPC...")
    
    channel = grpc.insecure_channel('localhost:50051')
    stub = telemetry_pb2_grpc.MimeTelemetryServiceStub(channel)
    
    try:
        # Subscribe to the fused Cartesian state
        stream = stub.SubscribeFusedState(telemetry_pb2.SubscribeRequest())
        
        print("[IK Engine] Connected! Listening for Cartesian coordinates...")
        
        for state in stream:
            # 1. Extract Target Position
            # We scale down the user's movements slightly so it fits inside the robot's physical reach envelope
            target_pos = [state.px * 0.5, state.py * 0.5, state.pz * 0.5 + 0.5] 
            
            # 2. Extract Target Orientation
            target_quat = [state.orientation.x, state.orientation.y, state.orientation.z, state.orientation.w]
            
            # 3. Calculate Inverse Kinematics!
            # This complex calculus takes the 3D target coordinate and figures out the 7 joint angles
            joint_angles = p.calculateInverseKinematics(
                bodyUniqueId=robot_id,
                endEffectorLinkIndex=end_effector_index,
                targetPosition=target_pos,
                targetOrientation=target_quat,
                maxNumIterations=100,
                residualThreshold=1e-4
            )
            
            # 4. Print the Joint Angles (In a real system, you would send these to the motor driver)
            angles_deg = [math.degrees(a) for a in joint_angles]
            print(f"[IK Angles] J1:{angles_deg[0]:.1f}° J2:{angles_deg[1]:.1f}° J3:{angles_deg[2]:.1f}° "
                  f"J4:{angles_deg[3]:.1f}° J5:{angles_deg[4]:.1f}° J6:{angles_deg[5]:.1f}° J7:{angles_deg[6]:.1f}°")
                  
    except grpc.RpcError as e:
        print(f"[IK Engine] gRPC Error: {e.details()}")
    finally:
        p.disconnect()

if __name__ == '__main__':
    main()
