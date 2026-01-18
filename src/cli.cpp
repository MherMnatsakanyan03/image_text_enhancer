#include <getopt.h>
#include <iostream>
#include <string>
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
    OPT_SAUVOLA_WINDOW,
    OPT_SAUVOLA_K,
    OPT_SAUVOLA_DELTA,
    OPT_TIME,
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
              << "  -t, --time                    Report execution time of enhance function\n"
              << "  -h, --help                    Show this help\n";
}

int main(int argc, char* argv[])
{
    ite::EnhanceOptions opt;
    std::string input_path, output_path;
    bool measure_time = false;

    // getopt settings:
    // - leading ':' => we handle missing arg as ':' return value
    // - opterr = 0 => we print our own errors
    opterr = 0;
    auto shortopts = ":i:o:th";

    const option longopts[] = {{"input", required_argument, nullptr, 'i'},
                               {"output", required_argument, nullptr, 'o'},
                               {"help", no_argument, nullptr, 'h'},
                               {"time", no_argument, nullptr, 't'},

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

        case OPT_SIGMA:
            {
                opt.sigma = parse_float(optarg, "--sigma");
                require_positive_f("--sigma", opt.sigma);
            }
            break;

        case OPT_SIGMA_LOW:
            {
                opt.adaptive_sigma_low = parse_float(optarg, "--sigma-low");
                require_positive_f("--sigma-low", opt.adaptive_sigma_low);
            }
            break;
        case OPT_SIGMA_HIGH:
            {
                opt.adaptive_sigma_high = parse_float(optarg, "--sigma-high");
                require_positive_f("--sigma-high", opt.adaptive_sigma_high);
            }
            break;
        case OPT_EDGE_THRESH:
            opt.adaptive_edge_thresh = parse_float(optarg, "--edge-thresh");
            break;

        case OPT_MEDIAN_SIZE:
            {
                opt.median_kernel_size = (int)parse_uint(optarg, "--median-size");
                require_positive("--median-size", opt.median_kernel_size);
            }
            break;

        case OPT_MEDIAN_THRESH:
            opt.median_threshold = (int)parse_uint(optarg, "--median-thresh");
            break;

        case OPT_ADAPTIVE_MEDIAN_MAX:
            {
                opt.adaptive_median_max_window = (int)parse_uint(optarg, "--adaptive-median-max");
                require_positive("--adaptive-median-max", opt.adaptive_median_max_window);
                if ((opt.adaptive_median_max_window % 2) == 0)
                    die_usage("--adaptive-median-max must be odd");
                if (opt.adaptive_median_max_window < 3)
                    die_usage("--adaptive-median-max must be >= 3");
            }
            break;

        case OPT_KERNEL_SIZE:
            {
                opt.kernel_size = (int)parse_uint(optarg, "--kernel-size");
                require_positive("--kernel-size", opt.kernel_size);
            }
            break;

        case OPT_DESPECKLE_THRESH:
            opt.despeckle_threshold = (int)parse_uint(optarg, "--despeckle-thresh");
            require_non_negative("--despeckle-thresh", opt.despeckle_threshold);
            break;

        case OPT_SAUVOLA_WINDOW:
            {
                opt.sauvola_window_size = (int)parse_uint(optarg, "--sauvola-window");
                require_positive("--sauvola-window", opt.sauvola_window_size);
            }
            break;

        case OPT_SAUVOLA_K:
            {
                opt.sauvola_k = parse_float(optarg, "--sauvola-k");
                require_positive_f("--sauvola-k", opt.sauvola_k);
            }
            break;

        case OPT_SAUVOLA_DELTA:
            opt.sauvola_delta = parse_float(optarg, "--sauvola-delta");
            break;

        case ':': // missing required argument
            die_usage(std::string("Missing value for option '-") + static_cast<char>(optopt) + "'");
            break;

        case '?':
            { // unknown option
                if (optopt)
                {
                    die_usage(std::string("Unknown option '-") + static_cast<char>(optopt) + "'");
                }
                else
                {
                    // For long options, getopt_long sets optopt to 0; argv[optind-1] is the token.
                    die_usage(std::string("Unknown option '") + argv[optind - 1] + "'");
                }
            }
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
        std::cout << "Processing: " << input_path << " -> " << output_path << std::endl;
        auto img = ite::loadimage(input_path);

        auto start = omp_get_wtime();
        auto result = ite::enhance(img, opt, 64);
        auto end = omp_get_wtime();

        ite::writeimage(result, output_path);
        std::cout << "Success!" << std::endl;

        if (measure_time)
        {
            std::cout << "Enhancement time: " << end-start << " s" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
