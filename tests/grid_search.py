import subprocess
import itertools
import os
import re
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np


def run_ite_visualizer():
    # --- CONFIGURATION ---
    input_file = "resources/bench.jpg"
    ite_binary = "./ite"
    output_dir = "test_results"

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Define your parameter matrix here.
    # Every key must match the CLI flag name (without the leading --).
    # Even fixed parameters should be in a list: [value]
    param_grid = {
        "do-gaussian": [False],                     # Fixed toggle
        "do-adaptive-gaussian": [True],
        "sigma": list(np.arange(0.5, 4.5, 0.5)),    # Variable: 0.5 to 4.0
        "do-median": [True, False],                 # Variable toggle
        "median-size": [3, 5],                      # Variable integers
        "sauvola-k": [0.2],                         # Fixed float
        "sauvola-window": [15],                     # Fixed int
        "do-despeckle": [True]                      
    }

    # --- EXECUTION LOGIC ---
    keys = list(param_grid.keys())
    combinations = list(itertools.product(*param_grid.values()))

    results = []  # Store (image_path, label, time)

    print(f"Running {len(combinations)} scenarios...")

    for i, values in enumerate(combinations):
        params = dict(zip(keys, values))

        # Build command
        cmd = [ite_binary, "-i", input_file, "-t"]
        file_tags = []

        for k, v in params.items():
            if isinstance(v, bool):
                if v:
                    cmd.append(f"--{k}")
                    file_tags.append(k)
            else:
                cmd.extend([f"--{k}", str(v)])
                file_tags.append(f"{k}:{v}")

        output_filename = f"out_{i:03d}_" + "_".join(file_tags) + ".png"
        output_path = os.path.join(output_dir, output_filename)
        cmd.extend(["-o", output_path])

        # Run and extract timing
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, check=True)

            # Regex to find "Enhancement time: 0.031 s"
            time_match = re.search(r"Enhancement time: ([\d.]+) s", proc.stdout)
            timing = time_match.group(1) if time_match else "N/A"

            results.append({
                "path": output_path,
                "label": "\n".join(file_tags),
                "time": timing
            })
            print(f"Finished {i + 1}/{len(combinations)} (Time: {timing}s)")

        except subprocess.CalledProcessError as e:
            print(f"Error in scenario {i}: {e.stderr}")

    # --- VISUALIZATION ---
    num_results = len(results)
    cols = 3
    rows = (num_results + cols - 1) // cols

    # Load first image to get dimensions for proper sizing
    if results:
        sample_img = mpimg.imread(results[0]["path"])
        img_height, img_width = sample_img.shape[:2]

        # Calculate figure size to maintain image aspect ratio and resolution
        # Increase the multiplier to show images at higher resolution
        fig_width = cols * (img_width / 100)  # Scale down by 100 for reasonable figure size
        fig_height = rows * (img_height / 100)

        # Ensure minimum reasonable size
        fig_width = max(fig_width, cols * 6)
        fig_height = max(fig_height, rows * 4)
    else:
        fig_width, fig_height = 15, 5 * rows

    fig, axes = plt.subplots(rows, cols, figsize=(fig_width, fig_height), dpi=150)
    fig.suptitle(f"ITE Parameter Comparison\nInput: {input_file}", fontsize=16)

    # Flatten axes for easy iteration if it's a 2D array
    axes_flat = axes.flatten() if num_results > 1 else [axes]

    for idx, res in enumerate(results):
        ax = axes_flat[idx]
        img = mpimg.imread(res["path"])

        # Use 'none' interpolation to maintain crisp pixels at full resolution
        ax.imshow(img, cmap='gray', interpolation='none')

        # Title includes parameters and the extracted timing
        ax.set_title(f"{res['label']}\nTime: {res['time']}s", fontsize=10, color='blue')
        ax.axis('off')

    # Hide unused subplots
    for j in range(idx + 1, len(axes_flat)):
        axes_flat[j].axis('off')

    # Adjust layout to prevent image compression
    plt.tight_layout(rect=[0, 0.03, 1, 0.95], pad=0.5)

    # Save high-resolution comparison grid
    comparison_path = os.path.join(output_dir, "comparison_grid.png")
    plt.savefig(comparison_path, dpi=300, bbox_inches='tight')
    print(f"High-resolution comparison grid saved to: {comparison_path}")

    plt.show()


if __name__ == "__main__":
    run_ite_visualizer()