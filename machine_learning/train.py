import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader
from dataset import MimeDataset
from model import MimeLSTM
import os

def train():
    # 1. Setup Hyperparameters
    batch_size = 32
    learning_rate = 0.001
    epochs = 50
    csv_dir = "../central_engine" # Where the DataLogger drops the CSV files
    
    print(f"[Train] Initializing dataset from {csv_dir}")
    dataset = MimeDataset(csv_dir=csv_dir, window_size=10)
    
    if len(dataset) == 0:
        print("[Train] ERROR: No CSV data found. Put on the glove and record some data first!")
        return
        
    dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
    print(f"[Train] Dataset loaded with {len(dataset)} samples.")
    
    # 2. Setup Model & Optimizer
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"[Train] Using device: {device}")
    
    model = MimeLSTM().to(device)
    criterion = nn.MSELoss() # Mean Squared Error (perfect for spatial coordinates)
    optimizer = optim.Adam(model.parameters(), lr=learning_rate)
    
    # 3. Training Loop
    print("[Train] Starting training...")
    for epoch in range(epochs):
        epoch_loss = 0.0
        for i, (inputs, targets) in enumerate(dataloader):
            inputs, targets = inputs.to(device), targets.to(device)
            
            # Zero gradients
            optimizer.zero_grad()
            
            # Forward pass
            outputs = model(inputs)
            
            # Calculate error
            loss = criterion(outputs, targets)
            
            # Backward pass and optimize
            loss.backward()
            optimizer.step()
            
            epoch_loss += loss.item()
            
        avg_loss = epoch_loss / len(dataloader)
        if (epoch+1) % 10 == 0:
            print(f"[Train] Epoch [{epoch+1}/{epochs}], Loss: {avg_loss:.6f}")
            
    # 4. Save the trained weights
    torch.save(model.state_dict(), "mime_model.pth")
    print("[Train] Training complete. Model saved to 'mime_model.pth'.")

if __name__ == "__main__":
    train()
