#pragma once
#include "CImg.h"
#include <string>

using namespace cimg_library;

namespace ite {

    CImg<unsigned char> loadimage(std::string filepath);
}