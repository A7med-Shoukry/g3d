
// Dis-allow compilation by default due to dependency on freeImage (while in development)
#if 0

#include "G3D/Image.h"


namespace G3D {

Image::Image()
    : m_image(NULL)
    , m_format(ImageFormat::AUTO()) {

    m_image = new fipImage;
}

Image::~Image() {
    if (m_image) {
        delete m_image;
    }
}

Image::Ref Image::fromFile(const std::string& filename, FREE_IMAGE_FORMAT fileFormat, const ImageFormat* imageFormat) {
    Image* img = new Image;

    debugAssert(fileFormat == FIF_UNKNOWN || fileFormat == fipImage::identifyFIF(filename.c_str()));

    if (! img->m_image->load(filename.c_str()))
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
    
    // Flip image to use g3d coordinates since we can't flip when uploading texture
    img->m_image->flipVertical();

    return img;
}

Image::Ref Image::fromInput(const BinaryInput& bi, FREE_IMAGE_FORMAT fileFormat, const ImageFormat* imageFormat) {
    return NULL;
}

void Image::get(const Point2int32& pos, Color4& color) const {
    Point2int32 fipPos(pos.x, m_image->getHeight() - pos.y - 1);

    BYTE* scanline = m_image->getScanLine(fipPos.y);
    switch (m_image->getImageType())
    {
        case FIT_BITMAP:
        {
            if (m_image->isGrayscale()) {
                color.r = (scanline[fipPos.x] / 255.0f);
                color.g = color.r;
                color.b = color.r;
                color.a = 1.0f;
            } else if (m_image->getBitsPerPixel() == 24) {
                scanline += 3 * fipPos.x;

                color.r = (scanline[FI_RGBA_RED] / 255.0f);
                color.g = (scanline[FI_RGBA_GREEN] / 255.0f);
                color.b = (scanline[FI_RGBA_BLUE] / 255.0f);
                color.a = 1.0f;
            } else if (m_image->getBitsPerPixel() == 32) {
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

void Image::get(const Point2int32& pos, Color3& color) const {
    Color4 c;
    get(pos, c);
    color = Color3(c.r, c.g, c.b);
}

void Image::get(const Point2int32& pos, Color4unorm8& color) const {
    Point2int32 fipPos(pos.x, m_image->getHeight() - pos.y - 1);

    BYTE* scanline = m_image->getScanLine(fipPos.y);
    switch (m_image->getImageType())
    {
        case FIT_BITMAP:
        {
            if (m_image->isGrayscale()) {
                color.r = unorm8::fromBits(scanline[fipPos.x]);
                color.g = color.r;
                color.b = color.r;
                color.a = unorm8::fromBits(255);
            } else if (m_image->getBitsPerPixel() == 24) {
                scanline += 3 * fipPos.x;

                color.r = unorm8::fromBits(scanline[FI_RGBA_RED]);
                color.g = unorm8::fromBits(scanline[FI_RGBA_GREEN]);
                color.b = unorm8::fromBits(scanline[FI_RGBA_BLUE]);
                color.a = unorm8::fromBits(255);
            } else if (m_image->getBitsPerPixel() == 32) {
                scanline += 4 * fipPos.x;

                color.r = unorm8::fromBits(scanline[FI_RGBA_RED]);
                color.g = unorm8::fromBits(scanline[FI_RGBA_GREEN]);
                color.b = unorm8::fromBits(scanline[FI_RGBA_BLUE]);
                color.a = unorm8::fromBits(scanline[FI_RGBA_ALPHA]);
            }
            break;
        }
    }
}

void Image::get(const Point2int32& pos, Color3unorm8& color) const {
    Color4unorm8 c;
    get(pos, c);
    color = c.rgb();
}

void Image::set(const Point2int32& pos, const Color4& color) {
    Point2int32 fipPos(pos.x, m_image->getHeight() - pos.y - 1);

    BYTE* scanline = m_image->getScanLine(fipPos.y);
    switch (m_image->getImageType())
    {
        case FIT_BITMAP:
        {
            if (m_image->isGrayscale()) {
                scanline[fipPos.x] = static_cast<BYTE>(iClamp(color.r * 255.0f, 0, 255));
            } else if (m_image->getBitsPerPixel() == 24) {
                scanline += 3 * fipPos.x;

                scanline[FI_RGBA_RED] = static_cast<BYTE>(iClamp(color.r * 255.0f, 0, 255));
                scanline[FI_RGBA_GREEN] = static_cast<BYTE>(iClamp(color.g * 255.0f, 0, 255));
                scanline[FI_RGBA_BLUE] = static_cast<BYTE>(iClamp(color.b * 255.0f, 0, 255));
            } else if (m_image->getBitsPerPixel() == 32) {
                scanline += 4 * fipPos.x;

                scanline[FI_RGBA_RED] = static_cast<BYTE>(iClamp(color.r * 255.0f, 0, 255));
                scanline[FI_RGBA_GREEN] = static_cast<BYTE>(iClamp(color.g * 255.0f, 0, 255));
                scanline[FI_RGBA_BLUE] = static_cast<BYTE>(iClamp(color.b * 255.0f, 0, 255));
                scanline[FI_RGBA_ALPHA] = static_cast<BYTE>(iClamp(color.a * 255.0f, 0, 255));
            }
            break;
        }
    }
}

void Image::set(const Point2int32& pos, const Color3& color) {
    set(pos, Color4(color));
}

void Image::set(const Point2int32& pos, const Color4unorm8& color) {
    Point2int32 fipPos(pos.x, m_image->getHeight() - pos.y - 1);

    BYTE* scanline = m_image->getScanLine(fipPos.y);
    switch (m_image->getImageType())
    {
        case FIT_BITMAP:
        {
            if (m_image->isGrayscale()) {
                scanline[fipPos.x] = color.r.bits();
            } else if (m_image->getBitsPerPixel() == 24) {
                scanline += 3 * fipPos.x;

                scanline[FI_RGBA_RED] = color.r.bits();
                scanline[FI_RGBA_GREEN] = color.g.bits();
                scanline[FI_RGBA_BLUE] = color.b.bits();
            } else if (m_image->getBitsPerPixel() == 32) {
                scanline += 4 * fipPos.x;

                scanline[FI_RGBA_RED] = color.r.bits();
                scanline[FI_RGBA_GREEN] = color.g.bits();
                scanline[FI_RGBA_BLUE] = color.b.bits();
                scanline[FI_RGBA_ALPHA] = color.a.bits();
            }
            break;
        }
    }
}

void Image::set(const Point2int32& pos, const Color3unorm8& color) {
    set(pos, Color4unorm8(color, unorm8::fromBits(255)));
}

ImageBuffer::Ref Image::copyBuffer() {
    ImageBuffer::Ref buffer = ImageBuffer::create(AlignedMemoryManager::create(), m_format, m_image->getWidth(), m_image->getHeight(), 1, 1);

    BYTE* pixels = m_image->accessPixels();
    if (pixels) {
        int rowStride = buffer->width() * (m_format->cpuBitsPerPixel / 8);

        for (int row = 0; row < buffer->height(); ++row) {
            System::memcpy(buffer->row(row), m_image->getScanLine(row), rowStride);
        }
    }

    return buffer;
}

const ImageFormat* Image::determineImageFormat() const {
    debugAssert(m_image->isValid() && m_image->getImageType() != FIT_UNKNOWN);
    
    const ImageFormat* imageFormat = NULL;
    switch (m_image->getImageType())
    {
        case FIT_BITMAP:
        {
            switch (m_image->getBitsPerPixel())
            {
                case 8:
                    imageFormat = ImageFormat::L8();
                    break;

                case 16:
                    // todo: find matching image format
                    break;

                case 24:
                    imageFormat = ImageFormat::BGR8();
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

    if (m_image->getColorType() != FIC_RGB && m_image->getColorType() != FIC_RGBALPHA) {
        debugAssertM(false, "Unsupported FreeImage color space loaded.");
        imageFormat = NULL;
    }

    return imageFormat;
}


} // namespace G3D

#endif // dis-allow compilation (while in development)
