#!/usr/bin/env python3
import json
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os
import sys

def parse_args():
    parser = argparse.ArgumentParser(description="Plot gain over line data from simulation result JSON files.")
    parser.add_argument("json_file", type=str, help="Path to the JSON data file.")
    parser.add_argument("-o", "--output", type=str, default=None, 
                        help="Path to save the output plot (e.g. plot.png). Default: <json_base_name>_plot.png")
    parser.add_argument("--db-ref", type=float, default=1.0, 
                        help="Reference value for decibel calculation (default: 1.0).")
    return parser.parse_args()

def main():
    args = parse_args()
    
    if not os.path.exists(args.json_file):
        print(f"Error: File '{args.json_file}' does not exist.")
        sys.exit(1)
        
    print(f"Loading data from {args.json_file}...")
    with open(args.json_file, 'r') as f:
        data = json.load(f)
        
    # Extract coordinates and gains
    positions = np.array(data['positions'])
    gains = np.array(data['gains'])
    name = data.get('name', 'Gain Plot')
    wavelength = data.get('wavelength', None)
    
    # 1. Automatically detect the evaluation axis (X, Y, or Z)
    # By finding peak-to-peak (ptp) range for each coordinate array
    ranges = [np.ptp(positions[:, i]) for i in range(3)]
    active_axis_idx = np.argmax(ranges)
    axis_names = ['X', 'Y', 'Z']
    active_axis_name = axis_names[active_axis_idx]
    active_coords = positions[:, active_axis_idx]
    
    # Calculate gain statistics
    real_gains = gains[:, 0]
    imag_gains = gains[:, 1]
    magnitude = np.sqrt(real_gains**2 + imag_gains**2)
    
    # Calculate decibels safely (handling potential zeros)
    with np.errstate(divide='ignore'):
        magnitude_db = 20 * np.log10(magnitude / args.db_ref)
        
    # Print the evaluation summary to the console
    print("-" * 55)
    print(f"Analysis Summary for: {name}")
    print(f"Detected Evaluation Axis : {active_axis_name}")
    print(f"Position Range on Axis   : {active_coords.min():.4f} to {active_coords.max():.4f} m (Total: {len(active_coords)} points)")
    if wavelength is not None:
        print(f"Wavelength (\u03bb)      : {wavelength} m")
    print(f"Max Gain Magnitude       : {magnitude.max():.6e}")
    print(f"Min Gain Magnitude       : {magnitude.min():.6e}")
    print(f"Mean Gain Magnitude      : {magnitude.mean():.6e}")
    print("-" * 55)
    
    # 2. Setup the Dual-Panel Plotting Layout
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5))
    
    # Panel 1: Linear Magnitude, Real, and Imaginary Components
    ax1.plot(active_coords, magnitude, label='Magnitude $|G|$', color='black', linewidth=2)
    ax1.plot(active_coords, real_gains, label='Real $\\Re(G)$', linestyle='--', color='royalblue', alpha=0.8)
    ax1.plot(active_coords, imag_gains, label='Imag $\\Im(G)$', linestyle=':', color='crimson', alpha=0.8)
    ax1.set_title(f"Linear Scale Gain vs {active_axis_name}-Axis", fontsize=11, fontweight='bold')
    ax1.set_xlabel(f"Position along {active_axis_name}-axis [m]")
    ax1.set_ylabel("Gain Scale")
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend(loc='best')
    
    # Panel 2: Decibel Scale Magnitude
    ax2.plot(active_coords, magnitude_db, label=f'Magnitude (dB, ref={args.db_ref})', color='forestgreen', linewidth=2)
    ax2.set_title(f"Decibel Scale Gain vs {active_axis_name}-Axis", fontsize=11, fontweight='bold')
    ax2.set_xlabel(f"Position along {active_axis_name}-axis [m]")
    ax2.set_ylabel("Gain [dB] ($20\\log_{10}(|G|)$)")
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend(loc='best')
    
    # Master title and configuration
    plt.suptitle(f"Gain Profile: {name}\nAligned Axis: {active_axis_name}", fontsize=13, y=0.98)
    plt.tight_layout()
    
    # Save the file
    output_path = args.output if args.output else f"{os.path.splitext(args.json_file)[0]}_plot.png"
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Plot successfully saved to: {output_path}\n")

if __name__ == '__main__':
    main()
