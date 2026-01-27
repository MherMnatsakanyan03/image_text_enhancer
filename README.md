# image_text_enhancer

Project for the module "Algorithmic Engineering".

In this project, we implement an image enhancement algorithm specifically designed to improve the readability of text in
images. The algorithm focuses on enhancing contrast and clarity, making it easier to extract and recognize text from
various types of images, such as scanned documents, photographs of signs, and other text-containing visuals.

## Overview

The Image Text Enhancer project now provides two executables:

1. **`ite`** (ITE_cli) - Command-line interface for processing single images with configurable options
2. **`ite-demo`** (ITE_demo) - Batch processing demo that processes all images in the resources/ directory

## Building the Project

```bash
mkdir -p build
cd build
cmake ..
make
```

This will create two executables in `build/src/`:

- `ite` - The CLI tool
- `ite-demo` - The demo/batch processing tool

## CLI Usage

### Basic Usage

```bash
./ite <input_image> <output_image> [options]
```

### Examples

#### Simple Gaussian Blur

```bash
./ite input.jpg output.jpg --gaussian 1.5
```

#### Adaptive Gaussian with Auto Parameters

```bash
./ite input.png output.png --auto-adaptive-gaussian
```

#### Text Enhancement Pipeline

```bash
./ite scanned_document.jpg enhanced.jpg --auto-adaptive-gaussian --despeckle --deskew
```

#### Multiple Filters

```bash
./ite input.jpg output.jpg --adaptive-gaussian --adaptive-sigma-low 0.3 --adaptive-sigma-high 2.5 --erosion --kernel-size 3
```

#### Median Filtering

```bash
./ite noisy.jpg clean.jpg --median 5
```

## Available Options

### General

- `-h, --help` - Show help message

### Gaussian Blur

- `--gaussian <sigma>` - Apply Gaussian blur with specified sigma value

### Median Blur

- `--median <size>` - Apply median blur with specified kernel size
- `--median-threshold <val>` - Median threshold value (default: 0)

### Adaptive Median

- `--adaptive-median` - Apply adaptive median filter
- `--adaptive-median-max <size>` - Maximum window size (default: 7)

### Adaptive Gaussian

- `--adaptive-gaussian` - Apply adaptive Gaussian blur
- `--adaptive-sigma-low <val>` - Low sigma value (default: 0.5)
- `--adaptive-sigma-high <val>` - High sigma value (default: 2.0)
- `--adaptive-edge-thresh <val>` - Edge threshold (default: 30.0)
- `--auto-adaptive-gaussian` - Automatically choose adaptive Gaussian parameters based on image content

### Morphological Operations

- `--erosion` - Apply erosion
- `--dilation` - Apply dilation
- `--kernel-size <size>` - Kernel size for morphological operations (default: 5)
- `--diagonal` - Use diagonal connections in morphological operations
- `--no-diagonal` - Don't use diagonal connections (default)

### Despeckling

- `--despeckle` - Apply despeckling to remove noise
- `--despeckle-threshold <val>` - Despeckle threshold (default: 0)

### Geometric Transformations

- `--deskew` - Apply automatic deskewing to straighten the image

### Binarization (Sauvola)

- `--sauvola-window <size>` - Sauvola window size (default: 15)
- `--sauvola-k <val>` - Sauvola k parameter (default: 0.2)
- `--sauvola-delta <val>` - Sauvola delta parameter (default: 0.0)

### Other

- `--boundary <mode>` - Boundary conditions (0=Dirichlet, 1=Neumann, default: 1)

## Demo Tool Usage

The demo tool processes all images in the `resources/` directory and saves them to `output/`:

```bash
./ite-demo
```

The demo tool uses predefined settings and is useful for batch processing or testing the library.

## Project Structure

```
image_text_enhancer/
├── CMakeLists.txt              # Root CMake configuration
├── src/
│   ├── CMakeLists.txt          # Source directory CMake (builds both executables)
│   ├── cli.cpp                 # CLI interface implementation
│   ├── main.cpp                # Demo/batch processing implementation
│   └── lib/                    # Library implementation
│       ├── CMakeLists.txt      # Library build configuration
│       ├── ite.h               # Main library header
│       ├── ite.cpp             # Main library implementation
│       ├── filters/            # Image filtering algorithms
│       ├── morphology/         # Morphological operations
│       ├── geometry/           # Geometric transformations
│       ├── binarization/       # Binarization algorithms
│       ├── color/              # Color space operations
│       ├── core/               # Core utilities
│       └── io/                 # Image I/O operations
├── tests/                      # Unit tests
├── resources/                  # Input images for demo
└── README.md                   # Main project README

```

## Library Usage

Both executables link against the `ITE_Libs` static library, which contains all image processing algorithms. The library
can be integrated into other C++ projects by:

1. Linking against `ITE_Libs`
2. Including the appropriate headers from `src/lib/`
3. Using the `ite::enhance()` function or individual filter functions

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
    
        
    // See library for parameter details.
    ite::EnhanceOptions enhance_opts = {
        // denoising / filtering
        .do_adaptive_median = true,
        .median_kernel_size = 3,
        .median_threshold = 0,
        .adaptive_median_max_window = 15,
        .diagonal_connections = true,
        
        // morphologic transformations
        .do_erosion = true,
        .do_dilation = true,
        .do_despeckle = true,
        .kernel_size = 5,
        .despeckle_threshold = 0,
        
        // geometric transformations
        .do_deskew = true,
        
        // binarization
        .sauvola_window_size = 15,
        .sauvola_k = 0.2f,
    };
    
    // The main enhancement function call. 
    auto enhanced_img = ite::enhance(img, enhance_opts);

    ite::writeimage(enhanced_img, (output_dir / "enhanced.jpg").string());
    std::cout << "Images saved to: " << output_dir.string() << std::endl;

    return 0;
}
```

As we can see from the example above, we use the `CImg` library to handle image loading and saving. The `ite::enhance`
function (hence, this library) requires `CImg` images as input and output.

The example with these parameters results in the following enhancement:

![Original](resources/stock_gs200.jpg)

![Enhanced](resources/enhanced.jpg)

## Contributors

- [Mher Mnatsakanyan](mailto:mher.mnatsakanyan@uni-jena.de)
- [Leonard Teschner](mailto:leonard.teschner@uni-jena.de)
- [Daniel Motz](mailto:daniel.motz@uni-jena.de)
