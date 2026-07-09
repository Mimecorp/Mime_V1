import torch
from model import MimeLSTM
import numpy as np
import time

def predict():
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    model = MimeLSTM().to(device)
    
    # Load trained weights
    try:
        model.load_state_dict(torch.load("mime_model.pth"))
        model.eval()
        print("[Inference] Loaded 'mime_model.pth'")
    except FileNotFoundError:
        print("[Inference] ERROR: Could not find 'mime_model.pth'. Run train.py first!")
        return

    print("[Inference] Generating autonomous trajectory...")
    
    # Let's start with a completely still hand at origin (zeroes)
    # The LSTM expects a window of 10 previous frames to generate the 11th frame.
    current_window = torch.zeros(1, 10, 12).to(device)
    
    # Generate 100 frames of autonomous movement (1 second of robot motion at 100Hz)
    with torch.no_grad():
        for i in range(100):
            # The AI hallucinates the very next frame
            prediction = model(current_window)
            
            # We slide the window forward!
            # Drop the oldest frame, and append the new AI prediction to the end of the history buffer
            prediction_unsqueeze = prediction.unsqueeze(1) # shape: (1, 1, 12)
            current_window = torch.cat((current_window[:, 1:, :], prediction_unsqueeze), dim=1)
            
            pred_np = prediction.cpu().numpy()[0]
            
            print(f"Frame {i}: Pos[X:{pred_np[0]:.2f}, Y:{pred_np[1]:.2f}, Z:{pred_np[2]:.2f}] "
                  f"Flex[Thumb:{pred_np[10]:.2f}, Index:{pred_np[11]:.2f}]")
            
            time.sleep(0.01) # Simulate 100Hz loop

if __name__ == "__main__":
    predict()
