import subprocess
import itertools
import os
import numpy as np

def run_ite_test_matrix():
    # --- CONFIGURATION ---
    input_file = "resources/bench.jpg"
    output_dir = "test_results"
    ite_binary = "./ite"

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Define your parameter matrix here.
    # Every key must match the CLI flag name (without the leading --).
    # Even fixed parameters should be in a list: [value]
    param_grid = {
        "do-gaussian": [True],                      # Fixed toggle
        "sigma": list(np.arange(0.5, 4.5, 0.5)),    # Variable: 0.5 to 4.0
        "do-median": [True, False],                 # Variable toggle
        "median-size": [3, 5],                      # Variable integers
        "sauvola-k": [0.2],                         # Fixed float
        "sauvola-window": [15],                     # Fixed int
        "do-despeckle": [True]                      # Fixed toggle
    }

    # --- LOGIC ---

    # 1. Prepare keys and the product of all variations
    keys = list(param_grid.keys())
    combinations = list(itertools.product(*param_grid.values()))

    print(f"Total scenarios to test: {len(combinations)}")
    print("-" * 30)

    for i, values in enumerate(combinations):
        # Map keys to the specific values for this iteration
        params = dict(zip(keys, values))

        # 2. Build the command and a unique filename
        cmd = [ite_binary, "-i", input_file]

        # Create a unique filename based on the parameters
        # Example: out_sigma1.5_medianTrue_size3.png
        file_tags = []

        for k, v in params.items():
            if isinstance(v, bool):
                if v: # Only add the flag if True
                    cmd.append(f"--{k}")
                    file_tags.append(f"{k}")
            else:
                cmd.extend([f"--{k}", str(v)])
                file_tags.append(f"{k}{v}")

        output_filename = f"out_{i:03d}_" + "_".join(file_tags) + ".png"
        output_path = os.path.join(output_dir, output_filename)

        cmd.extend(["-o", output_path])

        # 3. Execute
        print(f"[{i+1}/{len(combinations)}] Generating: {output_filename}")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            # print(result.stdout) # Uncomment if you want to see CLI output
        except subprocess.CalledProcessError as e:
            print(f"Error running scenario {i}: {e.stderr}")

    print("-" * 30)
    print(f"Done! Results saved to '{output_dir}/'")

if __name__ == "__main__":
    run_ite_test_matrix()