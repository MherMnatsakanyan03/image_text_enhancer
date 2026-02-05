#include <cmath>
#include <filesystem>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <omp.h>
#include <string>
#include <vector>
#include "ite.h"

// Define IDs for long-only options to keep the switch statement clean
enum : int
{
    OPT_DO_GAUSSIAN = 1000,
    OPT_DO_MEDIAN,
    OPT_DO_ADAPTIVE_MEDIAN,
    OPT_DO_ADAPTIVE_GAUSSIAN,
    OPT_DO_EROSION,
    OPT_DO_DILATION,
    OPT_DO_DESPECKLE,
    OPT_DO_DESKEW,
    OPT_DO_COLOR_PASS,
    OPT_SIGMA,
    OPT_SIGMA_LOW,
    OPT_SIGMA_HIGH,
    OPT_EDGE_THRESH,
    OPT_MEDIAN_SIZE,
    OPT_MEDIAN_THRESH,
    OPT_ADAPTIVE_MEDIAN_MAX,
    OPT_KERNEL_SIZE,
    OPT_DESPECKLE_THRESH,
    OPT_BINARIZATION_METHOD,
    OPT_SAUVOLA_WINDOW,
    OPT_SAUVOLA_K,
    OPT_SAUVOLA_DELTA,
    OPT_TRIALS,
    OPT_WARMUP,
    OPT_TIME_LIMIT
};

static void die_usage(const std::string &msg, int exit_code = 2)
{
    std::cerr << "Error: " << msg << "\n";
    std::cerr << "Try --help for usage.\n";
    std::exit(exit_code);
}

static unsigned int parse_uint(const char* s, const char* opt_name)
{
    // Reject negatives explicitly (strtoul would accept "-1" as huge)
    if (s[0] == '-')
        die_usage(std::string(opt_name) + " expects a non-negative integer (got '" + s + "')");

    errno = 0;
    char* end = nullptr;
    unsigned long v = std::strtoul(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0')
        die_usage(std::string(opt_name) + " expects a non-negative integer (got '" + s + "')");

    if (v > std::numeric_limits<unsigned int>::max())
        die_usage(std::string(opt_name) + " out of range (got '" + std::string(s) + "')");

    return static_cast<unsigned int>(v);
}

static float parse_float(const char* s, const char* opt_name)
{
    errno = 0;
    char* end = nullptr;
    float v = std::strtof(s, &end);

    if (errno != 0 || end == s || *end != '\0')
        die_usage(std::string(opt_name) + " expects a float (got '" + s + "')");

    return v;
}

static void require_positive(const char* opt_name, int v)
{
    if (v <= 0)
        die_usage(std::string(opt_name) + " must be > 0");
}

static void require_non_negative(const char* opt_name, int v)
{
    if (v < 0)
        die_usage(std::string(opt_name) + " must be >= 0");
}

static void require_positive_f(const char* opt_name, float v)
{
    if (v <= 0.0f)
        die_usage(std::string(opt_name) + " must be > 0");
}

static void print_help(const char* prog)
{
    const ite::EnhanceOptions d; // default values
    std::cout << "ITE - Image Text Enhancement CLI\n"
              << "Usage:\n"
              << "  " << prog << " -i <input> -o <output> [options]\n\n"
              << "Required:\n"
              << "  -i, --input <path>            Path to source image\n"
              << "  -o, --output <path>           Path to save processed result\n\n"

              << "GEOMETRY & PRE-PROCESSING:\n"
              << "  (Note: Contrast Stretching and Grayscale conversion are ALWAYS performed)\n"
              << "      --do-deskew               Straighten tilted text (default: " << (d.do_deskew ? "ON" : "OFF") << ")\n\n"

              << "DENOISING (Pre-Binarization):\n"
              << "      --do-gaussian             Apply Gaussian blur (default: " << (d.do_gaussian_blur ? "ON" : "OFF") << ")\n"
              << "      --sigma <float>           Gaussian sigma (default: " << d.sigma << ")\n"
              << "      --do-adaptive-gaussian    Apply adaptive blur [overrides --do-gaussian] (default: " << (d.do_adaptive_gaussian_blur ? "ON" : "OFF")
              << ")\n"
              << "      --sigma-low <float>       Adaptive low sigma (default: " << d.adaptive_sigma_low << ")\n"
              << "      --sigma-high <float>      Adaptive high sigma (default: " << d.adaptive_sigma_high << ")\n"
              << "      --edge-thresh <float>     Adaptive edge sensitivity (default: " << d.adaptive_edge_thresh << ")\n"
              << "      --do-median               Apply median filter (default: " << (d.do_median_blur ? "ON" : "OFF") << ")\n"
              << "      --median-size <int>       Median kernel size (default: " << d.median_kernel_size << ")\n"
              << "      --do-adaptive-median      Apply adaptive median filter (default: " << (d.do_adaptive_median ? "ON" : "OFF") << ")\n\n"

              << "BINARIZATION (Sauvola Algorithm):\n"
              << "      --binarization <name>         Method: otsu, sauvola, bataineh (default: bataineh)\n"
              << "      --sauvola-window <int>    Local window size (default: " << d.sauvola_window_size << ")\n"
              << "      --sauvola-k <float>       Sensitivity parameter k (default: " << d.sauvola_k << ")\n"
              << "      --sauvola-delta <float>   Threshold offset delta (default: " << d.sauvola_delta << ")\n\n"

              << "MORPHOLOGY (Post-Binarization):\n"
              << "      --do-despeckle            Remove small noise specks (default: " << (d.do_despeckle ? "ON" : "OFF") << ")\n"
              << "      --despeckle-thresh <int>  Max pixel size of specks to remove (default: " << d.despeckle_threshold << ")\n"
              << "      --do-dilation             Thicken/bolden dark features (default: " << (d.do_dilation ? "ON" : "OFF") << ")\n"
              << "      --do-erosion              Thin/shrink dark features (default: " << (d.do_erosion ? "ON" : "OFF") << ")\n"
              << "      --kernel-size <int>       Size of dilation/erosion square (default: " << d.kernel_size << ")\n\n"

              << "OUTPUT OPTIONS:\n"
              << "      --do-color-pass           Re-apply original color to binarized mask (default: " << (d.do_color_pass ? "ON" : "OFF") << ")\n"
              << "  -h, --help                    Show this help\n"
              << "  -v, --verbose                     Enable per-step timing output during execution\n"
              << "      --trials <int>                Number of trials for benchmark (default: 1)\n"
              << "      --warmup <int>                Number of warmup runs before benchmark\n"
              << "      --time-limit <int>        Max duration in minutes per image (default: 0 = no limit)\n";
}

void print_benchmark_table(const std::map<std::string, std::vector<double>> &aggregated_data, const std::vector<std::string> &step_order, int trials)
{
    std::cout << "\n" << std::string(85, '-') << "\n";
    std::cout << "BENCHMARK RESULTS (" << trials << " trials)\n";
    std::cout << std::string(85, '-') << "\n";
    std::cout << std::left << std::setw(30) << "Step" << std::right << std::setw(12) << "Avg (ms)" << std::setw(12) << "Min (ms)" << std::setw(12) << "Max (ms)"
              << std::setw(12) << "StdDev" << "\n"
              << std::string(85, '-') << "\n";

    for (const auto &step_name : step_order)
    {
        if (aggregated_data.find(step_name) == aggregated_data.end())
            continue;

        const auto &times = aggregated_data.at(step_name);
        if (times.empty())
            continue;

        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double mean = sum / times.size();

        double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
        double stdev = std::sqrt(std::max(0.0, sq_sum / times.size() - mean * mean));

        double min_v = *std::min_element(times.begin(), times.end());
        double max_v = *std::max_element(times.begin(), times.end());

        std::cout << std::left << std::setw(30) << step_name << std::right << std::fixed << std::setprecision(3) << std::setw(12) << mean << std::setw(12)
                  << min_v << std::setw(12) << max_v << std::setw(12) << stdev << "\n";
    }
    std::cout << std::string(85, '-') << "\n";
}

int main(int argc, char* argv[])
{
#ifdef _OPENMP
    std::cout << "_OPENMP defined. Max threads available: " << omp_get_max_threads() << "\n";
#endif

    ite::EnhanceOptions opt;
    std::string input_path, output_path;
    bool measure_time = false;
    bool verbose_log = false;
    int trials = 1;
    int warmup = 0;

    // getopt settings:
    // - leading ':' => we handle missing arg as ':' return value
    // - opterr = 0 => we print our own errors
    opterr = 0;
    auto shortopts = ":i:o:thv";

    const option longopts[] = {{"input", required_argument, nullptr, 'i'},
                               {"output", required_argument, nullptr, 'o'},
                               {"help", no_argument, nullptr, 'h'},
                               {"time", no_argument, nullptr, 't'},
                               {"verbose", no_argument, nullptr, 'v'},
                               {"trials", required_argument, nullptr, OPT_TRIALS},
                               {"warmup", required_argument, nullptr, OPT_WARMUP},
                               {"time-limit", required_argument, nullptr, OPT_TIME_LIMIT},

                               // Toggles
                               {"do-gaussian", no_argument, nullptr, OPT_DO_GAUSSIAN},
                               {"do-median", no_argument, nullptr, OPT_DO_MEDIAN},
                               {"do-adaptive-median", no_argument, nullptr, OPT_DO_ADAPTIVE_MEDIAN},
                               {"do-adaptive-gaussian", no_argument, nullptr, OPT_DO_ADAPTIVE_GAUSSIAN},
                               {"do-erosion", no_argument, nullptr, OPT_DO_EROSION},
                               {"do-dilation", no_argument, nullptr, OPT_DO_DILATION},
                               {"do-despeckle", no_argument, nullptr, OPT_DO_DESPECKLE},
                               {"do-deskew", no_argument, nullptr, OPT_DO_DESKEW},
                               {"do-color-pass", no_argument, nullptr, OPT_DO_COLOR_PASS},

                               // Values
                               {"binarization", required_argument, nullptr, OPT_BINARIZATION_METHOD},
                               {"sigma", required_argument, nullptr, OPT_SIGMA},
                               {"sigma-low", required_argument, nullptr, OPT_SIGMA_LOW},
                               {"sigma-high", required_argument, nullptr, OPT_SIGMA_HIGH},
                               {"edge-thresh", required_argument, nullptr, OPT_EDGE_THRESH},
                               {"median-size", required_argument, nullptr, OPT_MEDIAN_SIZE},
                               {"median-thresh", required_argument, nullptr, OPT_MEDIAN_THRESH},
                               {"adaptive-median-max", required_argument, nullptr, OPT_ADAPTIVE_MEDIAN_MAX},
                               {"kernel-size", required_argument, nullptr, OPT_KERNEL_SIZE},
                               {"despeckle-thresh", required_argument, nullptr, OPT_DESPECKLE_THRESH},
                               {"sauvola-window", required_argument, nullptr, OPT_SAUVOLA_WINDOW},
                               {"sauvola-k", required_argument, nullptr, OPT_SAUVOLA_K},
                               {"sauvola-delta", required_argument, nullptr, OPT_SAUVOLA_DELTA},

                               {nullptr, 0, nullptr, 0}};

    int c = 0;
    int time_limit_min = 0;
    while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) != -1)
    {
        switch (c)
        {
        case 'i':
            input_path = optarg;
            break;
        case 'o':
            output_path = optarg;
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        case 't':
            measure_time = true;
            break;
        case 'v':
            verbose_log = true;
            break;
        case OPT_TRIALS:
            trials = (int)parse_uint(optarg, "--trials");
            require_positive("--trials", trials);
            break;
        case OPT_WARMUP:
            warmup = (int)parse_uint(optarg, "--warmup");
            break;
        case OPT_TIME_LIMIT:
            time_limit_min = (int)parse_uint(optarg, "--time-limit");
            break;
        case OPT_DO_GAUSSIAN:
            opt.do_gaussian_blur = true;
            break;
        case OPT_DO_MEDIAN:
            opt.do_median_blur = true;
            break;
        case OPT_DO_ADAPTIVE_MEDIAN:
            opt.do_adaptive_median = true;
            break;
        case OPT_DO_ADAPTIVE_GAUSSIAN:
            opt.do_adaptive_gaussian_blur = true;
            break;
        case OPT_DO_EROSION:
            opt.do_erosion = true;
            break;
        case OPT_DO_DILATION:
            opt.do_dilation = true;
            break;
        case OPT_DO_DESPECKLE:
            opt.do_despeckle = true;
            break;
        case OPT_DO_DESKEW:
            opt.do_deskew = true;
            break;
        case OPT_DO_COLOR_PASS:
            opt.do_color_pass = true;
            break;
        case OPT_BINARIZATION_METHOD:
            {
                std::string method = optarg;
                for (auto &c : method)
                    c = tolower(c);

                if (method == "otsu")
                    opt.binarization_method = ite::BinarizationMethod::Otsu;
                else if (method == "sauvola")
                    opt.binarization_method = ite::BinarizationMethod::Sauvola;
                else if (method == "bataineh")
                    opt.binarization_method = ite::BinarizationMethod::Bataineh;
                else
                    die_usage("Unknown binarization method: " + method + " (allowed: otsu, sauvola, bataineh)");
                break;
            }
        case OPT_SIGMA:
            opt.sigma = parse_float(optarg, "--sigma");
            require_positive_f("--sigma", opt.sigma);
            break;
        case OPT_SIGMA_LOW:
            opt.adaptive_sigma_low = parse_float(optarg, "--sigma-low");
            require_positive_f("--sigma-low", opt.adaptive_sigma_low);
            break;
        case OPT_SIGMA_HIGH:
            opt.adaptive_sigma_high = parse_float(optarg, "--sigma-high");
            require_positive_f("--sigma-high", opt.adaptive_sigma_high);
            break;
        case OPT_EDGE_THRESH:
            opt.adaptive_edge_thresh = parse_float(optarg, "--edge-thresh");
            break;
        case OPT_MEDIAN_SIZE:
            opt.median_kernel_size = (int)parse_uint(optarg, "--median-size");
            require_positive("--median-size", opt.median_kernel_size);
            break;
        case OPT_MEDIAN_THRESH:
            opt.median_threshold = (int)parse_uint(optarg, "--median-thresh");
            break;
        case OPT_ADAPTIVE_MEDIAN_MAX:
            opt.adaptive_median_max_window = (int)parse_uint(optarg, "--adaptive-median-max");
            require_positive("--adaptive-median-max", opt.adaptive_median_max_window);
            if ((opt.adaptive_median_max_window % 2) == 0)
                die_usage("--adaptive-median-max must be odd");
            if (opt.adaptive_median_max_window < 3)
                die_usage("--adaptive-median-max must be >= 3");
            break;
        case OPT_KERNEL_SIZE:
            opt.kernel_size = (int)parse_uint(optarg, "--kernel-size");
            require_positive("--kernel-size", opt.kernel_size);
            break;
        case OPT_DESPECKLE_THRESH:
            opt.despeckle_threshold = (int)parse_uint(optarg, "--despeckle-thresh");
            require_non_negative("--despeckle-thresh", opt.despeckle_threshold);
            break;
        case OPT_SAUVOLA_WINDOW:
            opt.sauvola_window_size = (int)parse_uint(optarg, "--sauvola-window");
            require_positive("--sauvola-window", opt.sauvola_window_size);
            break;
        case OPT_SAUVOLA_K:
            opt.sauvola_k = parse_float(optarg, "--sauvola-k");
            require_positive_f("--sauvola-k", opt.sauvola_k);
            break;
        case OPT_SAUVOLA_DELTA:
            opt.sauvola_delta = parse_float(optarg, "--sauvola-delta");
            break;

        case ':':
            die_usage(std::string("Missing value for option '-") + static_cast<char>(optopt) + "'");
            break;
        case '?':
            die_usage(std::string("Unknown option"));
            break;
        default:
            die_usage("Unknown parsing error");
        }
    }

    if (input_path.empty() || output_path.empty())
    {
        print_help(argv[0]);
        return 0;
    }

    try
    {
        std::cout << "Loading: " << input_path << std::endl;
        auto img = ite::loadimage(input_path);

        std::filesystem::path p(input_path);
        std::cout << "Image Info: " << p.filename().string() << " (" << img.width() << "x" << img.height() << ", " << img.spectrum() << " channels)"
                  << std::endl;

        // Fix logic for Grayscale vs Color Pass
        if (img.spectrum() < 3 && opt.do_color_pass)
        {
            std::cout << "[INFO] Input image is grayscale (1 channel). Disabling --do-color-pass." << std::endl;
            opt.do_color_pass = false;
        }

        // --- WARMUP ---
        if (warmup > 0)
        {
            std::cout << "Warming up (" << warmup << " runs)..." << std::flush;
            for (int i = 0; i < warmup; ++i)
            {
                // Pass nullptr log, false verbose
                ite::enhance(img, opt, 64, nullptr, false);
            }
            std::cout << " Done." << std::endl;
        }

        // --- BENCHMARK ---
        std::map<std::string, std::vector<double>> aggregated_data;
        std::vector<std::string> step_order;
        ite::TimingLog log;
        log.reserve(20);

        std::cout << "Processing " << trials << " trial(s)..." << std::endl;

        // Setup for Progress Monitor
        using Clock = std::chrono::steady_clock;
        auto bench_start_time = Clock::now();
        int progress_update_freq = std::max(1, trials / 100);

        // for early finish
        double limit_seconds = time_limit_min * 60.0;
        int actual_trials = 0;

        CImg<uint> result;
        for (int i = 0; i < trials; ++i)
        {
            log.clear();

            // Only be verbose on the first trial if explicitly requested (to avoid spam)
            // But if verbose_log is true, we pass true.
            // If measure_time (-t) is true, we pass the log pointer.
            bool current_verbose = verbose_log && (i == 0 || trials == 1);

            result = ite::enhance(img, opt, 64, measure_time ? &log : nullptr, current_verbose);

            if (measure_time)
            {
                for (const auto &entry : log)
                {
                    aggregated_data[entry.name].push_back(entry.duration_us / 1000.0);
                    if (i == 0)
                    {
                        step_order.push_back(entry.name);
                    }
                }
            }

            actual_trials++;

            // --- TIME LIMIT CHECK ---
            auto now = Clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - bench_start_time).count();
            double elapsed_sec = elapsed_ms / 1000.0;

            if (time_limit_min > 0 && elapsed_sec >= limit_seconds)
            {
                if (!verbose_log)
                {
                    std::cerr << "\n[Benchmark] Time limit reached (" << time_limit_min << "m). Stopping early." << std::endl;
                }
                break;
            }

            if (!verbose_log && (i % progress_update_freq == 0 || i == trials - 1))
            {
                auto now = Clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - bench_start_time).count();

                int completed = i + 1;
                double avg_ms = (double)elapsed_ms / completed;
                double remaining_sec_trials = (avg_ms * (trials - completed)) / 1000.0;
                double remaining_sec = remaining_sec_trials;

                // If time limit is active, show the smaller remaining time
                if (time_limit_min > 0)
                {
                    double remaining_sec_time = limit_seconds - elapsed_sec;
                    if (remaining_sec_time < remaining_sec)
                        remaining_sec = remaining_sec_time;
                }

                remaining_sec = std::max(0.0, remaining_sec);

                std::filesystem::path p(input_path);
                std::string filename = p.filename().string();

                std::cerr << "\r[Benchmark: " << filename << "] " << int((float)completed / trials * 100.0) << "% " << "(" << completed << "/" << trials << ") "
                          << "ETA: " << std::fixed << std::setprecision(1) << remaining_sec << "s   " << std::flush;
            }
        }

        if (!verbose_log)
        {
            std::cerr << std::endl;
        }

        ite::writeimage(result, output_path);
        std::cout << "Saved: " << output_path << std::endl;

        if (measure_time)
        {
            print_benchmark_table(aggregated_data, step_order, actual_trials);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
