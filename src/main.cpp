#include <iostream>
#include "ite.h"

int main() {

    // Use the library function
    auto img = ite::loadimage("resources/stock_gs200.jpg");

    std::cout << "Image loaded with dimensions: " 
              << img.width() << "x" 
              << img.height() << "x" 
              << img.depth() << "x" 
              << img.spectrum() << std::endl;

    return 0;
}