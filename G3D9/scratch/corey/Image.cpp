
#include "Image.h"


namespace G3D {

Image::Image()
    : m_format(ImageFormat::AUTO()) {
}

Image::~Image() {

}

Image::Ref Image::fromFile(const std::string& filename, FREE_IMAGE_FORMAT fileFormat, const ImageFormat* imageFormat) {
    Image* img = new Image;

    debugAssert(fileFormat == FIF_UNKNOWN || fileFormat == fipImage::identifyFIF(filename.c_str()));

    if (! img->m_image.load(filename.c_str()))
    {
        delete img;
        img = NULL;
    }

    const ImageFormat* detectedFormat = img->determineImageFormat();
    
    if (! detectedFormat) {
        delete img;
        img = NULL;
    } 
    
    if (imageFormat == ImageFormat::AUTO()) {
        img->m_format = detectedFormat;
    } else {
        debugAssert(detectedFormat->canInterpretAs(imageFormat));
        img->m_format = imageFormat;
    }
    
    return img;
}

Image::Ref Image::fromInput(const BinaryInput& bi, FREE_IMAGE_FORMAT fileFormat, const ImageFormat* imageFormat) {
    return NULL;
}

void Image::get(const Point2int32& pos, Color4& color) const {
    Point2int32 fipPos(pos.x, m_image.getHeight() - pos.y - 1);

    BYTE* scanline = m_image.getScanLine(fipPos.y);
    switch (m_image.getImageType())
    {
        case FIT_BITMAP:
        {
            if (m_image.isGrayscale()) {
                color.r = (scanline[fipPos.x] / 255.0f);
                color.g = color.r;
                color.b = color.r;
                color.a = 1.0f;
            } else if (m_image.getBitsPerPixel() == 24) {
                scanline += 3 * fipPos.x;

                color.r = (scanline[FI_RGBA_RED] / 255.0f);
                color.g = (scanline[FI_RGBA_GREEN] / 255.0f);
                color.b = (scanline[FI_RGBA_BLUE] / 255.0f);
                color.a = 1.0f;
            } else if (m_image.getBitsPerPixel() == 32) {
                scanline += 4 * fipPos.x;

                color.r = (scanline[FI_RGBA_RED] / 255.0f);
                color.g = (scanline[FI_RGBA_GREEN] / 255.0f);
                color.b = (scanline[FI_RGBA_BLUE] / 255.0f);
                color.a = (scanline[FI_RGBA_ALPHA] / 255.0f);
            }
            break;
        }
    }
}

void Image::set(const Point2int32& pos, const Color4& color) {

}


const ImageFormat* Image::determineImageFormat() const {
    debugAssert(m_image.isValid() && m_image.getImageType() != FIT_UNKNOWN);
    
    const ImageFormat* imageFormat = NULL;
    switch (m_image.getImageType())
    {
        case FIT_BITMAP:
        {
            switch (m_image.getBitsPerPixel())
            {
                case 8:
                    imageFormat = ImageFormat::L8();
                    break;

                case 16:
                    // todo: find matching image format
                    break;

                case 24:
                    imageFormat = ImageFormat::RGB8();
                    break;

                case 32:
                    imageFormat = ImageFormat::RGBA8();
                    break;

                default:
                    debugAssertM(false, "Unsupported bit depth loaded.");
                    break;
            }
            break;
        }

        case FIT_UINT16:
        {
            imageFormat = ImageFormat::L16();
            break;
        }

        case FIT_INT16:
        case FIT_UINT32:
        case FIT_INT32:
        case FIT_FLOAT:
        case FIT_DOUBLE:
        case FIT_RGB16:
        case FIT_RGBA16:
        case FIT_RGBF:
        case FIT_RGBAF:
        case FIT_COMPLEX:
        default:
            debugAssertM(false, "Unsupported FreeImage type loaded.");
            break;
    }

    if (m_image.getColorType() != FIC_RGB && m_image.getColorType() != FIC_RGBALPHA) {
        debugAssertM(false, "Unsupported FreeImage color space loaded.");
        imageFormat = NULL;
    }

    return imageFormat;
}


} // namespace G3D
