#include <iostream>
#include <filesystem>
#include "ite.h"

int main(int argc, char* argv[]) {

    // Get the path to the directory the executable is in
    std::filesystem::path executable_path = (argc > 0) ? std::filesystem::canonical(argv[0]) : std::filesystem::current_path() / "ITE";
    std::filesystem::path base_dir = executable_path.parent_path();

    // Build paths relative to the executable's directory
    std::filesystem::path input_path = base_dir / "resources" / "stock_gs200.jpg";
    std::filesystem::path output_dir = base_dir / "output";

    auto img = ite::loadimage(input_path.string());

    std::cout << "Image loaded with dimensions: " 
              << img.width() << "x" 
              << img.height() << "x" 
              << img.depth() << "x" 
              << img.spectrum() << std::endl;

    // Create the directory if it doesn't exist
    if (!std::filesystem::is_directory(output_dir) && !std::filesystem::is_regular_file(output_dir)) {
        std::filesystem::create_directory(output_dir);
        std::cout << "Created directory: " << output_dir.string() << std::endl;
    }
    
    auto enhanced_img = ite::enhance(img, 0.0f, 1, 3, true, false, true, false, true, false);
    
    ite::writeimage(enhanced_img, (output_dir / "enhanced.jpg").string());
    std::cout << "Images saved to: " << output_dir.string() << std::endl;

    return 0;
}