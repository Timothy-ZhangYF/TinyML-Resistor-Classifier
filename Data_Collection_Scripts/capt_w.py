import serial
import time
import os
from datetime import datetime

# -------- CONFIG --------
SERIAL_PORT = r'\\.\COM11'
BAUD_RATE = 115200
OUTPUT_FOLDER = r'C:\Users\James\Downloads\Dataset'
NUM_FRAMES = 100

# Unique timestamp for this run
RUN_ID = datetime.now().strftime("%Y%m%d_%H%M%S")

# Make output folder
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# Open serial
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # wait for ESP32 reset

frame_count = 0
hex_buffer = ""
recording = False

# Start live mode
ser.write(b'live\n')
print(f"Run ID: {RUN_ID}")
print("Sent 'live' to ESP32, capturing frames...")

while frame_count < NUM_FRAMES:
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    if not line:
        continue

    if line == "---FRAME---":
        if hex_buffer:
            try:
                img_bytes = bytes.fromhex(hex_buffer)
                filename = os.path.join(
                    OUTPUT_FOLDER,
                    f"{RUN_ID}_frame_{frame_count:03d}.jpg"
                )
                with open(filename, "wb") as f:
                    f.write(img_bytes)
                print(f"Saved {filename} ({len(img_bytes)} bytes)")
                frame_count += 1
            except Exception as e:
                print("Error converting frame:", e)
            hex_buffer = ""
        recording = True
    elif recording:
        hex_buffer += line

# Stop live mode
ser.write(b'stop\n')
print("Capture complete. Stopped live mode.")
ser.close()
