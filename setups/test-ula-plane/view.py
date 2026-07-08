#!/usr/bin/env python3
import json
import sys
import matplotlib.pyplot as plt
import numpy as np

# 1. Load the JSON data file
file_path = sys.argv[1]

with open(file_path, "r") as f:
    data = json.load(f)

# 2. Extract structural boundaries from the metadata
axis1_name = data.get("label_axis1", "z")  # 'z'
axis2_name = data.get("label_axis2", "y")  # 'y'
n_points_1 = data["n_points_axis1"]
n_points_2 = data["n_points_axis2"]

print(
    f"Plotting plane: {axis2_name.upper()} (bottom) x {axis1_name.upper()} (left)"
)

# 3. Vectorized Loading of Complex Numbers
raw_floats = np.array(data["gains"], dtype=np.float64)
complex_gains = raw_floats.view(dtype=np.complex128).squeeze(-1)
gain_magnitudes = 20*np.log10(np.abs(complex_gains))

# --- SET PHYSICAL & VISUAL LIMITS SIMULTANEOUSLY ---
min_gain = -120  # Set your desired lower bound
max_gain = -20  # Set your desired upper bound

# Clip/Clamp the data arrays to your bounds
gain_magnitudes = np.clip(gain_magnitudes, min_gain, max_gain)

# 4. Extract Spatial Coordinates from Positions
axis_map = {"x": 0, "y": 1, "z": 2}
idx1 = axis_map[axis1_name.lower()]
idx2 = axis_map[axis2_name.lower()]

# Load raw positions matrix
positions = np.array(data["positions"], dtype=np.float64)

X_mesh = positions[:, :, idx1]  # Physical coordinates for axis1
Y_mesh = positions[:, :, idx2]  # Physical coordinates for axis2

# 5. Plotting the Heatmap
plt.figure(figsize=(9, 7))

# GENERIC SWAP: Pass Y_mesh (axis2) as the horizontal axis
# and X_mesh (axis1) as the vertical axis.
heatmap = plt.pcolormesh(
    Y_mesh, X_mesh, gain_magnitudes, shading="auto", cmap="viridis", vmin=min_gain, vmax=max_gain
)

# Customize Layout & Labels dynamically
plt.colorbar(heatmap, label="Absolute Gain Magnitude ($|g|$)")
plt.xlabel(f"{axis2_name.upper()} Position")  # Dynamically labels the bottom axis
plt.ylabel(f"{axis1_name.upper()} Position")  # Dynamically labels the left axis
plt.title(f"Heatmap of Gain Magnitudes: {data['name']}")
plt.grid(True, linestyle="--", alpha=0.3)

# Display the output
plt.tight_layout()
plt.show()
