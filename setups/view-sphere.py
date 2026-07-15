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

wavelength = data["num_params"]["wavelength"]
radius = data["spherical_rectangle"]["radius"]
ratio = radius / wavelength

# Extract structural grid dimensions from metadata
n_points_azimuth = data["num_params"]["n_azimuth"]
n_points_2 = data["num_params"]["n_polar"]

# 2. Extract Spherical Mesh Positions
positions = np.array(data["positions"], dtype=np.float64)

# Reshape the flattened JSON array back into its structural 2D shape (Height, Width, Coordinates)
positions = positions.reshape(n_points_azimuth, n_points_2, -1)

# Extracted mapping: index 0 is azimuth (phi), index 1 is polar angle (theta)
# Convert from radians to degrees for clearer plotting bounds
azimuth_mesh = np.degrees(positions[..., 0])
polar_mesh = np.degrees(positions[..., 1])

# 3. Vectorized Loading of Complex Numbers (Gain Data)
raw_floats = np.array(data["gains"], dtype=np.float64)
complex_gains = raw_floats.view(dtype=np.complex128).squeeze(-1)
gain_abs_max = np.max(np.abs(complex_gains))
#complex_gains /= gain_abs_max
complex_gains *= 4*np.pi*radius
gain_magnitudes = 20 * np.log10(np.abs(complex_gains))

# Reshape gain values to match the coordinate mesh grid layout
gain_magnitudes = gain_magnitudes.reshape(n_points_azimuth, n_points_2)



# 4. Set Physical & Visual Scaling Limits
min_gain = -20  # Clamping threshold for lower noise floor
max_gain = 20   # Maximum target visibility ceiling
gain_magnitudes = np.clip(gain_magnitudes, min_gain, max_gain)

# 5. Plotting the Spherical Heatmap
plt.figure(figsize=(9, 7))

# Mapping Azimuth to Horizontal axis (Axis 1) and Polar to Vertical axis (Axis 2)
heatmap = plt.pcolormesh(
    azimuth_mesh, polar_mesh, gain_magnitudes, shading="auto", cmap="viridis", vmin=min_gain, vmax=max_gain
)

# Customize Layout & Labels with accurate angle names
plt.colorbar(heatmap, label="Absolute Gain Magnitude ($|g|$) [dB]")
plt.xlabel(r"Azimuth Angle ($\phi$) [degrees]")
plt.ylabel(r"Polar Angle ($\theta$) [degrees]")
plt.title(f"Gain Distribution over Spherical Rectangle\nData: {data.get('name', 'Unknown')}\n" + r'$r/\lambda$ = ' + f'{ratio:.1f}')
plt.grid(True, linestyle="--", alpha=0.5)

# 6. Show / Save Result
plt.tight_layout()
plt.show()
