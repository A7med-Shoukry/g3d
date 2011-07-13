
#include "Image.h"


namespace G3D {

Image::Image() {
}

Image::~Image() {

}

Image::Ref Image::fromFile(const std::string& filename) {
    Image* img = new Image;

    if (! img->m_image.load(filename.c_str()))
    {
        delete img;
        img = NULL;
    }

    return img;
}



} // namespace G3D