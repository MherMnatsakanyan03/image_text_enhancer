# image_text_enhancer

Project for the module "Algorithmic Engineering".

In this project, we implement an image enhancement algorithm specifically designed to improve the readability of text in images. The algorithm focuses on enhancing contrast and clarity, making it easier to extract and recognize text from various types of images, such as scanned documents, photographs of signs, and other text-containing visuals.

## Example

The following example demonstrates how to use the `image_text_enhancer` library to enhance an image containing text.

```cpp
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

    // The main enhancement function call. See library for parameter details.
    auto enhanced_img = ite::enhance(img, 0.0f, 1, 3);

    ite::writeimage(enhanced_img, (output_dir / "enhanced.jpg").string());
    std::cout << "Images saved to: " << output_dir.string() << std::endl;

    return 0;
}
```

As we can see from the example above, we use the `CImg` library to handle image loading and saving. The `ite::enhance` function (hence, this library) requires `CImg` images as input and output.

The example with these parameters results in the following enhancement:

![Original](resources/stock_gs200.jpg)

![Enhanced](resources/enhanced.jpg)

## Contributors

- [Mher Mnatsakanyan](mailto:mher.mnatsakanyan@uni-jena.de)
- [Leonard Teschner](mailto:leonard.teschner@uni-jena.de)
- [Daniel Motz](mailto:daniel.motz@uni-jena.de)
