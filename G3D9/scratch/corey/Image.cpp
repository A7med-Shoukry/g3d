
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

void Image::get(const Point2int32& pos, Color3& color) const {
    RGBQUAD rgb;
    // TODO: Convert vertical (y) pixel coordinate to match FreeImage space (0 is bottom of image)
    if (m_image.getPixelColor(pos.x, pos.y, &rgb))
    {
        color.r = rgb.rgbRed / 255.0f;
        color.g = rgb.rgbGreen / 255.0f;
        color.b = rgb.rgbBlue / 255.0f;
    }
    else
    {
        debugAssertM(false, "Unable to access pixels from FreeImage format.");
    }
}

} // namespace G3D