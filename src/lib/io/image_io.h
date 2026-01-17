/**
 * @file image_io.h
 * @brief Image loading and saving utilities.
 */

#include <string>
#include "CImg.h"

using namespace cimg_library;

namespace ite::io
{

    /**
     * @brief Loads an image from a specified file path.
     * @param filepath The relative or absolute path to the image file.
     * @return A CImg<uint> object containing the image data.
     * @throws CImgIOException if the file cannot be opened or recognized.
     */
    CImg<uint> load_image(const std::string &filepath);

    /**
     * @brief Saves an image to a specified file path.
     * @param image The CImg<uint> object containing the image data to save.
     * @param filepath The relative or absolute path where the image will be saved.
     * @throws CImgIOException if the file cannot be written.
     */
    CImg<uint> save_image(const CImg<uint> &image, const std::string &filepath);

} // namespace ite::io
