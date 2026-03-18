import numpy as np
import pandas as pd
from scipy.optimize import curve_fit

def power_law(n_pixels, a, b):
    return a * n_pixels**b

# Path to your CSV file
csv_path = "bench_logs/bench_summary.csv"

# Read the CSV
df = pd.read_csv(csv_path)

# Sort by problem size just to make outputs easier to read
df = df.sort_values("total_pixels").reset_index(drop=True)

# Independent/dependent variables
N = df["total_pixels"].to_numpy(dtype=float)
T = df["avg_ms"].to_numpy(dtype=float)

# Optional: use the uncertainty of the mean runtime as fit weights
# stddev_ms is the sample stddev across trials, so SEM = stddev / sqrt(trials)
sem = (df["stddev_ms"] / np.sqrt(df["trials"])).to_numpy(dtype=float)

# Reasonable initial guess
a0 = T[0] / N[0]
b0 = 1.0

# Fit the power law
params, cov = curve_fit(
    power_law,
    N,
    T,
    p0=(a0, b0),
    sigma=sem,
    absolute_sigma=True,
    maxfev=10000
)

a, b = params
a_err, b_err = np.sqrt(np.diag(cov))

print(f"a = {a:.6e} ± {a_err:.6e}")
print(f"b = {b:.6f} ± {b_err:.6f}")

print(f"\nEstimated model:")
print(f"runtime_ms ≈ {a:.6e} * total_pixels^{b:.6f}")

print(f"\nScaling interpretation:")
print(f"- Doubling total pixels multiplies runtime by about {2**b:.3f}x")
print(f"- Doubling width and height (4x pixels) multiplies runtime by about {4**b:.3f}x")

# Predicted values for each row
df["predicted_ms"] = power_law(N, a, b)
df["ratio_actual_to_predicted"] = df["avg_ms"] / df["predicted_ms"]

print("\nData with fitted predictions:")
print(df[[
    "image_name",
    "total_pixels",
    "avg_ms",
    "predicted_ms",
    "ratio_actual_to_predicted"
]].to_string(index=False))