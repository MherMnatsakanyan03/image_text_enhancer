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

    # Helper to generate combinations from a specific sub-grid
    def get_combos(grid):
        keys = list(grid.keys())
        values = list(grid.values())
        return [dict(zip(keys, v)) for v in itertools.product(*values)]

    # --- DEFINE LOGICAL SCENARIOS ---
    all_scenarios = []

    # Shared parameters used in all tests
    base_params = {
        "sauvola-k": [0.2],
        "sauvola-window": [15],
        "do-despeckle": [False]
    }

    # Scenario 1: Adaptive Median is ON (Sweep the max window sizes)
    all_scenarios.extend(get_combos({
        **base_params,
        "do-adaptive-median": [True],
        "adaptive-median-max": list(np.arange(3, 16, 2)),
    }))

    # Scenario 2: Adaptive Median is OFF (Only run once, ignore max window)
    all_scenarios.extend(get_combos({
        **base_params,
        "do-adaptive-median": [False],
    }))

    # Example: How to add Adaptive Gaussian logic easily later
    # all_scenarios.extend(get_combos({
    #     **base_params,
    #     "do-adaptive-gaussian": [True],
    #     "sigma-low": [0.5, 1.0],
    #     "sigma-high": [2.0, 3.0]
    # }))

    # --- EXECUTION LOGIC ---
    results = []
    print(f"Running {len(all_scenarios)} scenarios...")

    for i, params in enumerate(all_scenarios):
        # Build command
        cmd = [ite_binary, "-i", input_file, "-t"]
        file_tags = []

        # Construct flags based on the dictionary content
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

        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
            time_match = re.search(r"Enhancement time: ([\d.]+) s", proc.stdout)
            timing = time_match.group(1) if time_match else "N/A"

            results.append({
                "path": output_path,
                "label": "\n".join(file_tags),
                "time": timing
            })
            print(f"Finished {i + 1}/{len(all_scenarios)} (Time: {timing}s)")

        except subprocess.CalledProcessError as e:
            print(f"Error in scenario {i}: {e.stderr}")

    # --- VISUALIZATION ---
    if not results:
        print("No results to display.")
        return

    num_results = len(results)
    cols = 4  # Increased columns for better grid fit
    rows = (num_results + cols - 1) // cols

    sample_img = mpimg.imread(results[0]["path"])
    img_height, img_width = sample_img.shape[:2]
    fig_width = max(float(cols * 4), cols * (img_width / 100))
    fig_height = max(float(rows * 3), rows * (img_height / 100))

    fig, axes = plt.subplots(rows, cols, figsize=(fig_width, fig_height), dpi=150)
    fig.suptitle(f"ITE Parameter Comparison\nInput: {input_file}", fontsize=14)

    axes_flat = axes.flatten() if num_results > 1 else [axes]

    for idx, res in enumerate(results):
        ax = axes_flat[idx]
        img = mpimg.imread(res["path"])
        ax.imshow(img, cmap='gray', interpolation='none')
        ax.set_title(f"{res['label']}\n{res['time']}s", fontsize=8, color='blue')
        ax.axis('off')

    for j in range(idx + 1, len(axes_flat)):
        axes_flat[j].axis('off')

    plt.tight_layout(rect=(0, 0.03, 1, 0.95))
    comparison_path = os.path.join(output_dir, "comparison_grid.png")
    plt.savefig(comparison_path, dpi=200, bbox_inches='tight')
    print(f"Saved: {comparison_path}")
    plt.show()

if __name__ == "__main__":
    run_ite_visualizer()