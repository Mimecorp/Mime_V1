const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const express = require('express');
const { Server } = require('socket.io');
const http = require('http');
const path = require('path');

// Load the exact same protobuf we use in the C++ engine
const PROTO_PATH = path.join(__dirname, '../../central_engine/proto/telemetry.proto');

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});
const telemetryProto = grpc.loadPackageDefinition(packageDefinition).mime.telemetry;

const app = express();
const server = http.createServer(app);

// Allow our Vite frontend to connect from port 5173
const io = new Server(server, {
  cors: { origin: '*' }
});

// Connect directly to the C++ Central Engine
const client = new telemetryProto.MimeTelemetryService('localhost:50051', grpc.credentials.createInsecure());

console.log("Connecting to C++ Central Engine via gRPC...");

// Open the Server-Side Streaming RPC to get continuous math updates
const call = client.SubscribeFusedState({});

call.on('data', (fusedState) => {
    // When the C++ engine gives us a new state, instantly blast it to the browser over WebSockets!
    io.emit('telemetry', fusedState);
});

call.on('error', (err) => {
    console.error("gRPC Error:", err.message);
});

call.on('end', () => {
    console.log("gRPC stream ended.");
});

// Start the WebSocket server
server.listen(3001, () => {
    console.log("Bridge listening on port 3001. Awaiting browser WebGL connections...");
});
