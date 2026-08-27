"""
SEDS BPHC - Avionics Induction Task 1: Finding the Sea Floor
Author: Saptarshi Nandi
ID: [2026A4PS0609H]
"""

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import pandas as pd

# ---------------------------------------------------------
# 1. Grab & Clean Data
# ---------------------------------------------------------
def load_and_preprocess_data(file_path):
    # read the CSV with header
    df = pd.read_csv(file_path)

    # 1. Extract and convert depth to numeric (turns '#VALUE!' string into NaN)
    # Convert to positive values (meters below surface)
    df["depth_raw"] = pd.to_numeric(df["Depth (m)"], errors="coerce").abs()
    df["time"] = np.arange(len(df))

    # 2. Corrupted Data Handling: Remove extreme spikes (like the -1271.1m glitch)
    rolling_median = df["depth_raw"].rolling(window=7, center=True).median()     # I learnt abt rolling median from this line...
    diff = (df["depth_raw"] - rolling_median).abs()
    std = df["depth_raw"].std()
    
    # Mark points deviating by > 3 standard deviations from local median as corrupted
    df.loc[diff > (3 * std), "depth_raw"] = np.nan

    # Interpolate missing and corrupted data points linearly
    df["depth_cleaned"] = df["depth_raw"].interpolate(method="linear").bfill().ffill()

    # 3. Noise Reduction : Smoothing out random sensor jitter
    df["depth_smoothed"] = df["depth_cleaned"].rolling(window=5, min_periods=1, center=True).mean()

    return df

# ---------------------------------------------------------
# 2. Visualization Setup
# ---------------------------------------------------------
DATA_FILE = "depth_data.csv"
df = load_and_preprocess_data(DATA_FILE)

time_data = df["time"].values
raw_data = df["depth_raw"].values
smoothed_data = df["depth_smoothed"].values

fig, ax = plt.subplots(figsize=(11, 6))
plt.style.use("seaborn-v0_8-darkgrid" if "seaborn-v0_8-darkgrid" in plt.style.available else "default")


(line_raw,) = ax.plot([], [], label="Raw Sensor Readings", color="#e74c3c", alpha=0.35, linestyle="--", linewidth=1)
(line_smooth,) = ax.plot([], [], label="Filtered Seabed Profile", color="#2980b9", linewidth=2.2)

# Graph limits
ax.set_xlim(0, len(time_data))
# Surface is at top (0m), deep sea downwards... since depth is measured downwards and y = 0 is taken as the surface.
y_max = np.nanmax(smoothed_data)
ax.set_ylim(0, y_max * 1.15)
ax.invert_yaxis()

# Labels and Styling
ax.set_title("Odysseus Vessel - Real-Time Sea Floor Depth Monitoring", fontsize=13, fontweight="bold")
ax.set_xlabel("Time (seconds)", fontsize=11)
ax.set_ylabel("Depth Below Surface (meters)", fontsize=11)
ax.legend(loc="upper right", frameon=True)
ax.grid(True, linestyle=":", alpha=0.6)

# Real-time Telemetry Box(This is where the graph plotting happens)
status_text = ax.text(0.02, 0.93, "", transform=ax.transAxes, fontsize=10, fontweight="medium",
                      bbox=dict(boxstyle="round,pad=0.5", facecolor="white", edgecolor="#bdc3c7", alpha=0.9))

def init():
    line_raw.set_data([], [])
    line_smooth.set_data([], [])
    status_text.set_text("")
    return line_raw, line_smooth, status_text

def update(frame):
    current_time = time_data[:frame + 1]
    current_raw = raw_data[:frame + 1]
    current_smooth = smoothed_data[:frame + 1]

    line_raw.set_data(current_time, current_raw)
    line_smooth.set_data(current_time, current_smooth)

    latest_depth = smoothed_data[frame]
    status_text.set_text(f"Telemetry | Time: {frame}s | Filtered Depth: {latest_depth:.2f} m")

    return line_raw, line_smooth, status_text

# Animate point-by-point(it's getting animated at 1 frame per secpond)
ani = animation.FuncAnimation(
    fig,
    update,
    frames=len(time_data),
    init_func=init,
    interval=1000,
    blit=False,
    repeat=False
)

if __name__ == "__main__":
    plt.tight_layout()
    plt.show()
    
