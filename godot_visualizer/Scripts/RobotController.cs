using Godot;
using System;

public partial class RobotController : Node3D
{
    [Export]
    public NodePath TelemetryClientPath;

    private TelemetryClient _telemetryClient;

    public override void _Ready()
    {
        if (TelemetryClientPath != null)
        {
            _telemetryClient = GetNode<TelemetryClient>(TelemetryClientPath);
        }
        else 
        {
            GD.PrintErr("TelemetryClientPath is not assigned on RobotController!");
        }
    }

    public override void _Process(double delta)
    {
        // This runs every single frame (e.g., 60 or 144 times a second)
        if (_telemetryClient != null && _telemetryClient.LatestState != null)
        {
            var state = _telemetryClient.LatestState;
            
            // Map the FusedState positions from the Math Engine to the Godot Engine.
            // (You may need to invert axes depending on how the C++ Engine is oriented vs Godot's Y-up, right-handed system)
            Vector3 newPosition = new Vector3((float)state.Px, (float)state.Py, (float)state.Pz);
            
            // Map the Quaternions
            Quaternion newRotation = new Quaternion(
                (float)state.Orientation.X, 
                (float)state.Orientation.Y, 
                (float)state.Orientation.Z, 
                (float)state.Orientation.W
            );

            // Instantly apply the physics tracking to the 3D Mesh
            this.Position = newPosition;
            this.Quaternion = newRotation;
        }
    }
}
