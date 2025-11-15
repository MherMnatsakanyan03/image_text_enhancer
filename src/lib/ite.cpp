#include "ite.h"

namespace ite {
    CImg<unsigned char> loadimage(std::string filepath) {
        CImg<unsigned char> image(filepath.c_str());
        return image;
    }
}