#!/usr/bin/env python3
import json
import sys
import matplotlib.pyplot as plt
import numpy as np

# 1. Load the JSON data file
if len(sys.argv) < 2:
    print("Usage: ./view.py <path_to_json_file>")
    sys.exit(1)

file_path = sys.argv[1]

with open(file_path, "r") as f:
    data = json.load(f)

num_params = data["num_params"]

task = data["task"]

# Extract structural boundaries from the metadata
n_points_1 = task["n_dim1"]
n_points_2 = task["n_dim2"]

# 2. Function to dynamically detect active axes
def detect_axes_from_positions(positions_array):
    """
    Detects the two active scanning axes from a 3D positions array.
    Assumes the scan plane is aligned with two of the coordinate axes (X, Y, or Z).
    """
    # Flatten grid dimensions to get a list of 3D points: (NumPoints, 3)
    flat_positions = positions_array.reshape(-1, 3)

    # Calculate the range (max - min) along X (0), Y (1), and Z (2) axes
    ranges = np.ptp(flat_positions, axis=0)

    axis_names = ['x', 'y', 'z']

    # The constant axis (perpendicular to scan plane) has the smallest range
    constant_axis_idx = np.argmin(ranges)

    # The other two axes are our active scanning plane axes
    active_indices = [i for i in [0, 1, 2] if i != constant_axis_idx]

    axis1_name = axis_names[active_indices[0]]
    axis2_name = axis_names[active_indices[1]]

    return axis1_name, axis2_name

# 3. Load raw positions matrix
positions_raw = np.array(data["positions"]["cartesian"])
# If positions is flat/2D, reshape it to the 3D grid layout (N1, N2, 3 coordinates)
if positions_raw.ndim == 2:
    positions = positions_raw.reshape(n_points_1, n_points_2, 3)
else:
    positions = positions_raw


# Automatically detect active axes
axis1_name, axis2_name = detect_axes_from_positions(positions)

# Extract Spatial Coordinates from Positions using detected axes
axis_map = {"x": 0, "y": 1, "z": 2}
idx1 = axis_map[axis1_name]
idx2 = axis_map[axis2_name]

X_mesh = positions[:, :, idx1]  # Physical coordinates for axis1
Y_mesh = positions[:, :, idx2]  # Physical coordinates for axis2

# 4. Vectorized Loading of Complex Numbers
raw_floats = np.array(data["data"][0], dtype=np.float64)
complex_gains = raw_floats.view(dtype=np.complex128).squeeze(-1)
gain_magnitudes = 20 * np.log10(np.abs(complex_gains))

# --- RESHAPE TO 2D GRID ---
# Match the expected shape of your mesh grids
try:
    gain_magnitudes = gain_magnitudes.reshape(n_points_1, n_points_2)
except ValueError:
    # If the layout is transposed in JSON, fallback to transposed dimensions
    gain_magnitudes = gain_magnitudes.reshape(n_points_2, n_points_1)

# --- SET PHYSICAL & VISUAL LIMITS SIMULTANEOUSLY ---
min_gain = -120  # Set your desired lower bound
max_gain = -60   # Set your desired upper bound

# Clip/Clamp the data arrays to your bounds
gain_magnitudes = np.clip(gain_magnitudes, min_gain, max_gain)

# 5. Plotting the Heatmap
plt.figure(figsize=(9, 7))

# Pass Y_mesh (axis2) as the horizontal axis and X_mesh (axis1) as the vertical axis.
heatmap = plt.pcolormesh(
    Y_mesh, X_mesh, gain_magnitudes, shading="auto", cmap="viridis", vmin=min_gain, vmax=max_gain
)

# Customize Layout & Labels dynamically
plt.colorbar(heatmap, label="Absolute Gain Magnitude ($|g|$)")
plt.xlabel(f"{axis2_name.upper()} Position")  # Dynamically labels the bottom axis
plt.ylabel(f"{axis1_name.upper()} Position")  # Dynamically labels the left axis
plt.title(f"Heatmap of Gain Magnitudes: {data.get('name', 'Gain Map')}")
plt.grid(True, linestyle="--", alpha=0.3)

# Display the output
plt.tight_layout()
plt.show()
