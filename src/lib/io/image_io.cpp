#include "image_io.h"


namespace ite::io
{

    CImg<uint> load_image(const std::string &filepath) { return CImg<uint>(filepath.c_str()); }

    CImg<uint> save_image(const CImg<uint> &image, const std::string &filepath) { return image.save(filepath.c_str()); }

} // namespace ite::io
