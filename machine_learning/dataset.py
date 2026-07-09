import torch
from torch.utils.data import Dataset
import pandas as pd
import numpy as np
import os

class MimeDataset(Dataset):
    def __init__(self, csv_dir, window_size=10):
        self.window_size = window_size
        self.samples = []
        
        # We need these columns: px, py, pz, vx, vy, vz, q_w, q_x, q_y, q_z, flex_thumb, flex_index
        # We ignore timestamp for the network input
        
        # Load all CSVs in the directory
        if os.path.exists(csv_dir):
            for file in os.listdir(csv_dir):
                if file.endswith('.csv'):
                    df = pd.read_csv(os.path.join(csv_dir, file))
                    
                    # Ensure the CSV has data
                    if len(df) <= self.window_size:
                        continue
                        
                    # Drop timestamp, keep purely numerical kinematic data
                    data = df.drop(columns=['timestamp_ns']).values.astype(np.float32)
                    
                    # Create sliding windows
                    # X = history of N frames
                    # Y = the very next frame
                    for i in range(len(data) - self.window_size - 1):
                        X = data[i : i + self.window_size]
                        Y = data[i + self.window_size]
                        self.samples.append((X, Y))
                        
    def __len__(self):
        return len(self.samples)
        
    def __getitem__(self, idx):
        X, Y = self.samples[idx]
        return torch.tensor(X), torch.tensor(Y)

if __name__ == "__main__":
    print("[Dataset] This is a module. Import MimeDataset from here.")
