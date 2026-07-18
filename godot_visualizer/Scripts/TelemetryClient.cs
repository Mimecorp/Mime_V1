using Godot;
using System;
using System.Threading;
using System.Threading.Tasks;
using Grpc.Net.Client;
using Mime.Telemetry;

public partial class TelemetryClient : Node
{
    private GrpcChannel _channel;
    private MimeTelemetryService.MimeTelemetryServiceClient _client;
    private CancellationTokenSource _cancellationTokenSource;

    // We expose the latest fused state for the 3D objects to read
    public FusedState LatestState { get; private set; }

    public override void _Ready()
    {
        GD.Print("Connecting to Central Engine gRPC...");
        
        // Connect to the C++ Engine (adjust address if running on different devices)
        _channel = GrpcChannel.ForAddress("http://localhost:50051");
        _client = new MimeTelemetryService.MimeTelemetryServiceClient(_channel);
        _cancellationTokenSource = new CancellationTokenSource();

        // Run the blocking gRPC stream on a background thread so it doesn't freeze the Godot UI
        Task.Run(() => StreamDataAsync(_cancellationTokenSource.Token));
    }

    private async Task StreamDataAsync(CancellationToken token)
    {
        try
        {
            using var call = _client.SubscribeFusedState(new SubscribeRequest(), cancellationToken: token);
            while (await call.ResponseStream.MoveNext(token))
            {
                // Instantly grab the new state directly from C++ RAM/Network
                LatestState = call.ResponseStream.Current;
            }
        }
        catch (Exception e)
        {
            GD.PrintErr($"gRPC Stream Error: {e.Message}");
        }
    }

    public override void _ExitTree()
    {
        GD.Print("Shutting down gRPC connection...");
        _cancellationTokenSource?.Cancel();
        _channel?.Dispose();
    }
}
