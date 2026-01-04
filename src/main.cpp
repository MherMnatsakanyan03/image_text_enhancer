#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include "ite.h"

static bool is_image_file(const std::filesystem::path &p)
{
    if (!std::filesystem::is_regular_file(p))
        return false;

    std::string ext = p.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tif" || ext == ".tiff" || ext == ".gif";
}

int main(int argc, char* argv[])
{
    // Print OpenMP status
    std::cout << "cimg_use_openmp = " << cimg_use_openmp << "\n";
    std::cout << "CImg openmp_mode() = " << cimg::openmp_mode() << "\n";

#ifdef _OPENMP
    std::cout << "_OPENMP = " << _OPENMP << "\n";
#else
    std::cout << "_OPENMP is NOT defined\n";
#endif

    // Get the path to the directory the executable is in
    std::filesystem::path executable_path = (argc > 0) ? std::filesystem::canonical(argv[0]) : (std::filesystem::current_path() / "ITE");
    std::filesystem::path base_dir = executable_path.parent_path();

    // Input: all images in resources/
    std::filesystem::path input_dir = base_dir / "resources";

    // Output: output/
    std::filesystem::path output_dir = base_dir / "output";

    if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir))
    {
        std::cerr << "Input directory not found: " << input_dir.string() << std::endl;
        return 1;
    }

    // Create output directory if it doesn't exist
    if (!std::filesystem::exists(output_dir))
    {
        std::filesystem::create_directories(output_dir);
        std::cout << "Created directory: " << output_dir.string() << std::endl;
    }

    std::size_t processed = 0;
    for (const auto &entry : std::filesystem::directory_iterator(input_dir))
    {
        const auto &in_path = entry.path();
        if (!is_image_file(in_path))
            continue;

        try
        {
            auto img = ite::loadimage(in_path.string());

            std::cout << "Loaded: " << in_path.filename().string() << " (" << img.width() << "x" << img.height() << "x" << img.depth() << "x" << img.spectrum()
                      << ")\n";

            auto output_img = ite::enhance(img, 1.0f, 1, 2, true, true, false, true, true);

            // Save with the same filename into output/
            std::filesystem::path out_path = output_dir / in_path.filename();
            ite::writeimage(output_img, out_path.string());

            std::cout << "Saved:  " << out_path.string() << "\n";
            ++processed;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed processing " << in_path.string() << " : " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Failed processing " << in_path.string() << " : unknown error" << std::endl;
        }
    }

    std::cout << "Done. Processed " << processed << " image(s). Output dir: " << output_dir.string() << std::endl;

    return 0;
}
