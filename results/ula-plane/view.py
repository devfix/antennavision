#!/usr/bin/env python3
import json
import matplotlib.pyplot as plt
import numpy as np

# 1. Load the JSON data file
file_path = "plot_gain_over_plane.ula1.receiver.201.201.z.y.json"

with open(file_path, "r") as f:
    data = json.load(f)

# 2. Extract structural boundaries from the metadata
axis1_name = data.get("label_axis1", "z") # 'z'
axis2_name = data.get("label_axis2", "y") # 'y'
n_points_1 = data["n_points_axis1"]       # 101 points
n_points_2 = data["n_points_axis2"]       # 101 points

print(f"Plotting plane: {axis1_name.upper()} ({n_points_1} pts) x {axis2_name.upper()} ({n_points_2} pts)")

# 3. Vectorized Loading of Complex Numbers (Fixes the TypeError)
# Load raw float structure straight into NumPy. Shape will be (101, 101, 2)
raw_floats = np.array(data["gains"], dtype=np.float64)

# Re-interpret the last axis (the real/imag pair) as a single complex128 number
# This scales perfectly without manual python loops
complex_gains = raw_floats.view(dtype=np.complex128).squeeze(-1)

# Compute the Absolute Value (Magnitude) of the complex numbers
gain_magnitudes = np.abs(complex_gains)

# 4. Generate the proper spatial coordinate grid matching your labels
axis1_coords = np.arange(n_points_1)
axis2_coords = np.arange(n_points_2)
X_mesh, Y_mesh = np.meshgrid(axis1_coords, axis2_coords, indexing="ij")

# 5. Plotting the Heatmap
plt.figure(figsize=(9, 7))

# pcolormesh accurately draws matrix blocks across the coordinate grid
heatmap = plt.pcolormesh(
    X_mesh, Y_mesh, gain_magnitudes, shading="auto", cmap="viridis"
)

# Customize Layout & Labels using the file's native definitions
plt.colorbar(heatmap, label="Absolute Gain Magnitude ($|g|$)")
plt.xlabel(f"{axis1_name.upper()} Grid Index")
plt.ylabel(f"{axis2_name.upper()} Grid Index")
plt.title(f"Heatmap of Gain Magnitudes: {data['name']}")
plt.grid(True, linestyle="--", alpha=0.3)

# Display the output
plt.tight_layout()
plt.show()
