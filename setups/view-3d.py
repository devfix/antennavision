#!/usr/bin/env python3
import json
import sys
import matplotlib.pyplot as plt
import numpy as np

# 1. Parse Command Line Arguments
if len(sys.argv) < 3:
    print("Usage: ./view_rectangle.py <path_to_json_file> <sweep_index>")
    sys.exit(1)

file_path = sys.argv[1]
try:
    sweep_idx = int(sys.argv[2])
except ValueError:
    print("Error: <sweep_index> must be an integer.")
    sys.exit(1)

# 2. Load the JSON data file
with open(file_path, "r") as f:
    data = json.load(f)

num_params = data["num_params"]
n_points_1 = num_params["n_linear1"]
n_points_2 = num_params["n_linear2"]

# Extract sweep values
sweep_values = data["sweep"]["values"]
if sweep_idx < 0 or sweep_idx >= len(sweep_values):
    print(f"Error: sweep_index {sweep_idx} is out of bounds (0 to {len(sweep_values)-1}).")
    sys.exit(1)

wavelength = sweep_values[sweep_idx]

# 3. Load & Reshape Positions Matrix: Scheme -> [row][col][xyz]
positions_raw = np.array(data["positions"], dtype=np.float64)
if positions_raw.ndim == 2:
    positions = positions_raw.reshape(n_points_1, n_points_2, 3)
else:
    positions = positions_raw

# Extract 3D spatial coordinate meshes
X_mesh = positions[:, :, 0]
Y_mesh = positions[:, :, 1]
Z_mesh = positions[:, :, 2]

# 4. Extract Data: Scheme -> [sweep_idx][row][col][re/im]
raw_floats = np.array(data["data"][sweep_idx], dtype=np.float64)

# View float pairs as complex128 numbers and reshape to [row][col]
complex_gains = raw_floats.view(dtype=np.complex128).squeeze(-1)
complex_gains = complex_gains.reshape(n_points_1, n_points_2)

# Calculate gain magnitude in dB
gain_magnitudes = 20 * np.log10(np.abs(complex_gains))

# --- SET PHYSICAL & VISUAL LIMITS ---
min_gain = -120  # Lower bound (dB)
max_gain = -20   # Upper bound (dB)
gain_magnitudes = np.clip(gain_magnitudes, min_gain, max_gain)

# 5. Plotting the 3D Rectangular Surface
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection="3d")

# Normalize gain values to [0, 1] for mapping to the colormap
norm = plt.Normalize(vmin=min_gain, vmax=max_gain)
colors = plt.cm.viridis(norm(gain_magnitudes))

# Plot the 3D surface rectangle colored by gain magnitudes
surf = ax.plot_surface(
    X_mesh, Y_mesh, Z_mesh,
    facecolors=colors,
    rstride=1,
    cstride=1,
    shade=False,
    antialiased=True
)

# Add colorbar
sm = plt.cm.ScalarMappable(cmap="viridis", norm=norm)
sm.set_array([])
cbar = fig.colorbar(sm, ax=ax, shrink=0.7, aspect=15, pad=0.1)
cbar.set_label("Absolute Gain Magnitude ($|g|$) [dB]")

# Ensure equal aspect ratio in 3D so the rectangle is not distorted
x_ptp = np.ptp(X_mesh)
y_ptp = np.ptp(Y_mesh)
z_ptp = np.ptp(Z_mesh)
ax.set_box_aspect((
    max(x_ptp, 1e-5),
    max(y_ptp, 1e-5),
    max(z_ptp, 1e-5)
))

# Customize Layout & Labels
ax.set_xlabel("X Position")
ax.set_ylabel("Y Position")
ax.set_zlabel("Z Position")
ax.set_title(
    f"3D Rectangular Surface Gain @ λ = {wavelength} m (Sweep Index: {sweep_idx})"
)

plt.tight_layout()
plt.show()
