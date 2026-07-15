#!/usr/bin/env python3
import json
import math
from typing import List, Union, Tuple, Dict
import numpy as np
import matplotlib.pyplot as plt


def generate_upa_dft_codebook(
    N: int,
    M: int,
    oversampling_factor: Union[float, Tuple[float, float]],
    dx: float,
    dy: float,
    distance_ratios: List[Union[float, str]],
    output_filename: str = "upa_dft_codebook.json"
) -> Dict:
    """Generates a Near-Field/Far-Field DFT codebook for a Uniform Planar Array

    (UPA) and exports it as a JSON file in [abs, angle] polar format.

    Parameters:
        N (int): Number of antenna elements along the row dimension (x-axis).
        M (int): Number of antenna elements along the column dimension (y-axis).
        oversampling_factor (float | Tuple[float, float]): Oversampling factor(s)
          for DFT grid. If scalar, applied to both dimensions.
        dx (float): Element spacing along rows normalized by wavelength.
        dy (float): Element spacing along columns normalized by wavelength.
        distance_ratios (List[float | str]): Normalized focus distances (r /
          lambda). Use float('inf') or 'inf' for standard far-field planar
          beams.
        output_filename (str): Path for the exported JSON file.
        precision (int): Decimal precision for rounding float values in JSON.

    Returns:
        Dict: The generated codebook dictionary.
    """
    # 1. Resolve oversampling factors
    if isinstance(oversampling_factor, (int, float)):
        ox = oy = float(oversampling_factor)
    else:
        ox, oy = oversampling_factor

    N_grid = int(np.round(N * ox))
    M_grid = int(np.round(M * oy))

    # 2. Antenna element coordinates (normalized by wavelength, centered at origin)
    x = (np.arange(N) - (N - 1) / 2.0) * dx
    y = (np.arange(M) - (M - 1) / 2.0) * dy
    X, Y = np.meshgrid(x, y, indexing="ij")  # Shape: (N, M)

    # 3. Directional cosine grid (u, v) centered around 0
    u = (np.arange(N_grid) - N_grid // 2) / (N_grid * dx)
    v = (np.arange(M_grid) - M_grid // 2) / (M_grid * dy)

    codebook = {}

    print(f"Generating UPA Codebook: {N}x{M} elements -> Grid: {N_grid}x{M_grid}")
    print(f"Distance Ratios to process: {distance_ratios}")

    # 4. Generate codebook across all distance ratios, rows, and columns
    for ratio in distance_ratios:
        if isinstance(ratio, str) and ratio.lower() in ["inf", "infinity"]:
            gamma = float("inf")
            ratio_key = "inf"
        else:
            gamma = float(ratio)
            ratio_key = f"{gamma:.4g}"

        codebook[ratio_key] = {}

        for k in range(N_grid):
            row_key = str(k)
            codebook[ratio_key][row_key] = {}
            u_val = u[k]

            for l in range(M_grid):
                col_key = str(l)
                v_val = v[l]

                # Check visible physical region
                sin_sq = u_val**2 + v_val**2
                w_val = np.sqrt(max(0.0, 1.0 - sin_sq))

                # Calculate phase shifts
                if math.isinf(gamma):
                    # Far-field planar wavefront approximation
                    phase = 2 * np.pi * (u_val * X + v_val * Y)
                else:
                    # Exact near-field spherical wavefront
                    D = np.sqrt(
                        (gamma * u_val - X) ** 2
                        + (gamma * v_val - Y) ** 2
                        + (gamma * w_val) ** 2
                    )
                    phase = 2 * np.pi * (gamma - D)

                # Complex beamforming weights normalized by total elements
                weights = np.exp(1j * phase) / np.sqrt(N * M)

                # Flatten 2D array to 1D vector and format as [abs, angle_in_radians]
                weights_flat = weights.flatten()
                serializable_weights = [
                    [
                        float(np.abs(w)),
                        float(np.angle(w)),
                    ]
                    for w in weights_flat
                ]

                codebook[ratio_key][row_key][col_key] = serializable_weights

    # 5. Export to JSON
    with open(output_filename, "w", encoding="utf-8") as f:
        json.dump(codebook, f, indent=2)

    print(f"Successfully saved codebook to '{output_filename}'.")
    return codebook


def plot_beam_pattern(
    codebook: Dict,
    ratio_key: str,
    row_idx: int,
    col_idx: int,
    N: int,
    M: int,
    dx: float,
    dy: float,
    resolution: int = 200,
    dynamic_range_db: float = 40.0,
):
    """Visualizes the 2D Array Factor (AF) of a selected beam in the u-v

    directional cosine space.

    Parameters:
        codebook (Dict): The generated codebook dictionary.
        ratio_key (str): The distance ratio key as stored in the codebook (e.g.,
          "inf" or "10").
        row_idx (int): Row index of the beam to plot.
        col_idx (int): Column index of the beam to plot.
        N (int): Number of array elements along x-axis.
        M (int): Number of array elements along y-axis.
        dx (float): Normalized spacing along x-axis.
        dy (float): Normalized spacing along y-axis.
        resolution (int): Grid resolution for the u-v space evaluation.
        dynamic_range_db (float): Minimum dB floor for the colormap (default:
          -40 dB).
    """
    try:
        # Extract polar weights [abs, angle] from codebook
        polar_weights = codebook[str(ratio_key)][str(row_idx)][str(col_idx)]
    except KeyError:
        print(
            f"Error: Beam not found for ratio={ratio_key}, row={row_idx}, col={col_idx}"
        )
        return

    # Reconstruct complex weight matrix of shape (N, M)
    w_flat = np.array([mag * np.exp(1j * ang) for mag, ang in polar_weights])
    W = w_flat.reshape((N, M))

    # Array element positions
    x = (np.arange(N) - (N - 1) / 2.0) * dx
    y = (np.arange(M) - (M - 1) / 2.0) * dy
    X, Y = np.meshgrid(x, y, indexing="ij")

    # Generate fine u-v grid over [-1, 1]
    u_lin = np.linspace(-1.0, 1.0, resolution)
    v_lin = np.linspace(-1.0, 1.0, resolution)
    U, V = np.meshgrid(u_lin, v_lin, indexing="xy")

    # Mask out non-physical invisible region where u^2 + v^2 > 1
    visible_mask = (U**2 + V**2) <= 1.0

    # Calculate Array Factor: AF(u,v) = sum( W * exp(j * 2pi * (u*X + v*Y)) )
    # Using tensordot for efficient multi-dimensional inner product over antenna elements
    phase_matrix = 2 * np.pi * (np.multiply.outer(U, X) + np.multiply.outer(V, Y))
    steering_vectors = np.exp(1j * phase_matrix)  # Shape: (res, res, N, M)

    AF = np.tensordot(steering_vectors, W, axes=([2, 3], [0, 1]))
    AF_mag = np.abs(AF)

    # Normalize to peak and convert to dB
    peak_val = np.max(AF_mag) if np.max(AF_mag) > 0 else 1e-12
    AF_db = 20 * np.log10(np.clip(AF_mag / peak_val, 10 ** (-dynamic_range_db / 20), 1))
    AF_db[~visible_mask] = np.nan  # Hide invisible region

    # Plotting
    plt.figure(figsize=(8, 7))
    cmap = plt.cm.jet
    cmap.set_bad(color="white")  # Invisible region background

    im = plt.imshow(
        AF_db,
        extent=[-1, 1, -1, 1],
        origin="lower",
        cmap=cmap,
        vmin=-dynamic_range_db,
        vmax=0,
    )

    # Add visible region unit circle outline
    circle = plt.Circle(
        (0, 0), 1.0, color="black", fill=False, linestyle="--", linewidth=1.5
    )
    plt.gca().add_patch(circle)

    plt.colorbar(im, label="Normalized Array Factor (dB)")
    plt.title(f"UPA Beam Pattern (u-v space)\nRatio: {ratio_key} | Beam: ({row_idx}, {col_idx})")
    plt.xlabel("Directional Cosine $u = \\sin\\theta \\cos\\phi$")
    plt.ylabel("Directional Cosine $v = \\sin\\theta \\sin\\phi$")
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.xlim([-1.05, 1.05])
    plt.ylim([-1.05, 1.05])
    plt.tight_layout()
    plt.show()


# ==========================================
# Example Usage
# ==========================================
if __name__ == "__main__":
    # Define Array parameters
    NUM_ROWS = 8
    NUM_COLS = 8
    OVERSAMPLING = 2.0  # Creates a 16x16 beam grid
    SPACING_X = 0.5
    SPACING_Y = 0.5
    DIST_RATIOS = [5.0, 10.0, 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, "inf"]

    # 1. Generate codebook
    cb = generate_upa_dft_codebook(
        N=NUM_ROWS,
        M=NUM_COLS,
        oversampling_factor=OVERSAMPLING,
        dx=SPACING_X,
        dy=SPACING_Y,
        distance_ratios=DIST_RATIOS,
    )

    # 2. Visualize a far-field beam steered away from broadside
    # For a 16x16 grid, index (8, 8) is broadside (u=0, v=0). Let's plot (12, 10)
    plot_beam_pattern(
        codebook=cb,
        ratio_key="inf",
        row_idx=12,
        col_idx=10,
        N=NUM_ROWS,
        M=NUM_COLS,
        dx=SPACING_X,
        dy=SPACING_Y,
        dynamic_range_db=30.0,
    )
