import torch
import torch.nn as nn

class MimeLSTM(nn.Module):
    def __init__(self, input_size=12, hidden_size=64, num_layers=2, output_size=12):
        super(MimeLSTM, self).__init__()
        
        self.hidden_size = hidden_size
        self.num_layers = num_layers
        
        # The LSTM layer understands temporal sequences
        # input_size = 12 (px, py, pz, vx, vy, vz, qw, qx, qy, qz, thumb, index)
        self.lstm = nn.LSTM(input_size, hidden_size, num_layers, batch_first=True)
        
        # A fully connected layer to map the hidden state back to a 12-DOF output coordinate
        self.fc = nn.Linear(hidden_size, output_size)
        
    def forward(self, x):
        # x is a batch of sequences: (batch_size, window_size, input_size)
        
        # Initialize hidden state and cell state with zeros
        h0 = torch.zeros(self.num_layers, x.size(0), self.hidden_size).to(x.device)
        c0 = torch.zeros(self.num_layers, x.size(0), self.hidden_size).to(x.device)
        
        # Forward propagate LSTM
        out, _ = self.lstm(x, (h0, c0))
        
        # We only care about the output of the very last time step in the window
        out = out[:, -1, :]
        
        # Decode the hidden state into the physical coordinate predictions
        out = self.fc(out)
        return out

if __name__ == "__main__":
    print("[Model] Model compiled successfully.")
    # Quick sanity check
    model = MimeLSTM()
    dummy_input = torch.randn(1, 10, 12) # batch=1, window=10, features=12
    output = model(dummy_input)
    print(f"[Model] Input Shape: {dummy_input.shape} -> Output Shape: {output.shape}")
