
// Dis-allow compilation by default due to dependency on freeImage (while in development)
#if 0

#include "FreeImagePlus.h"
#include "G3D/Image.h"


namespace G3D {

static FREE_IMAGE_TYPE determineFreeImageType(const ImageFormat* imageFormat);
static FREE_IMAGE_FORMAT convertFileFormatToFIFormat(Image::FileFormat fileFormat);

Image::Image()
    : m_image(NULL)
    , m_format(ImageFormat::AUTO()) {

    // todo: if g3d ever has a global init, then this would move there to avoid deinitializing before program exit
    FreeImage_Initialise();
    m_image = new fipImage;
}

Image::~Image() {
    if (m_image) {
        delete m_image;
    }
    // This call can deinitialize the plugins if it's the last reference, but they can be re-initialized
    FreeImage_DeInitialise();
}

Image::Ref Image::fromFile(const std::string& filename, Image::FileFormat fileFormat, const ImageFormat* imageFormat) {
    Image* img = new Image;

    FREE_IMAGE_FORMAT fiFormat = convertFileFormatToFIFormat(fileFormat);
    debugAssert(fiFormat == FIF_UNKNOWN || fiFormat == fipImage::identifyFIF(filename.c_str()));

    if (! img->m_image->load(filename.c_str()))
    {
        delete img;
        return NULL;
    }

    const ImageFormat* detectedFormat = img->determineImageFormat();
    
    if (! detectedFormat) {
        delete img;
        return NULL;
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

void Image::toFile(const std::string& filename) const {
    if (! m_image->save(filename.c_str())) {
        debugAssertM(false, format("Failed to write image to %s", filename.c_str()));
    }
}

Image::Ref Image::fromBuffer(const ImageBuffer::Ref& buffer) {
    FREE_IMAGE_TYPE fiType = determineFreeImageType(buffer->format());
    debugAssertM(fiType != FIT_UNKNOWN, "Trying to create Image from unsupported ImageBuffer format %s");

    if (fiType == FIT_UNKNOWN) {
        return NULL;
    }

    Image* img = new Image;
    if (! img->m_image->setSize(fiType, buffer->width(), buffer->height(), buffer->format()->cpuBitsPerPixel)) {
        delete img;
        return NULL;
    }

    BYTE* pixels = img->m_image->accessPixels();
    debugAssert(pixels);

    if (pixels) {
        int rowStride = buffer->width() * (buffer->format()->cpuBitsPerPixel / 8);

        for (int row = 0; row < buffer->height(); ++row) {
            System::memcpy(img->m_image->getScanLine(row), buffer->row(row), rowStride);
        }
    }

    return img;
}

ImageBuffer::Ref Image::toBuffer() const {
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
            imageFormat = ImageFormat::L16();
            break;

        case FIT_RGBF:
            imageFormat = ImageFormat::RGB32F();
            break;

        case FIT_RGBAF:
            imageFormat = ImageFormat::RGBA32F();
            break;

        case FIT_INT16:
        case FIT_UINT32:
        case FIT_INT32:
        case FIT_FLOAT:
        case FIT_DOUBLE:
        case FIT_RGB16:
        case FIT_RGBA16:
        case FIT_COMPLEX:
        default:
            debugAssertM(false, "Unsupported FreeImage type loaded.");
            break;
    }

    if (m_image->getColorType() != FIC_RGB && m_image->getColorType() != FIC_RGBALPHA &&
        m_image->getColorType() != FIC_MINISBLACK && m_image->getColorType() != FIC_MINISWHITE) {
        debugAssertM(false, "Unsupported FreeImage color space loaded.");
        imageFormat = NULL;
    }

    return imageFormat;
}

static FREE_IMAGE_TYPE determineFreeImageType(const ImageFormat* imageFormat) {
    FREE_IMAGE_TYPE fiType = FIT_UNKNOWN;
    if (imageFormat == NULL) {
        return fiType;
    }

    switch (imageFormat->code) {
        case ImageFormat::CODE_L8:
        case ImageFormat::CODE_RGB8:
        case ImageFormat::CODE_RGBA8:
            fiType = FIT_BITMAP;
            break;

        case ImageFormat::CODE_L16:
            fiType = FIT_UINT16;
            break;

        case ImageFormat::CODE_RGB32F:
            fiType = FIT_RGBF;
            break;

        case ImageFormat::CODE_RGBA32F:
            fiType = FIT_RGBAF;
            break;

        default:
            break;
    }

    return fiType;
}

// Converts from Image::FileFormat to FREE_IMAGE_FORMAT respecting uncommon (casted) values
static FREE_IMAGE_FORMAT convertFileFormatToFIFormat(Image::FileFormat fileFormat) {
    FREE_IMAGE_FORMAT fiFormat = FIF_UNKNOWN;
    switch (fileFormat) {
        case Image::FILEFORMAT_AUTO:   fiFormat = FIF_UNKNOWN; break;
        case Image::FILEFORMAT_BMP:    fiFormat = FIF_BMP; break;
	    case Image::FILEFORMAT_ICO:    fiFormat = FIF_ICO; break;
	    case Image::FILEFORMAT_JPEG:   fiFormat = FIF_JPEG; break;
	    case Image::FILEFORMAT_MNG:    fiFormat = FIF_MNG; break;
	    case Image::FILEFORMAT_PBM:    fiFormat = FIF_PBM; break;
	    case Image::FILEFORMAT_PBMRAW: fiFormat = FIF_PBMRAW; break;
	    case Image::FILEFORMAT_PCX:    fiFormat = FIF_PCX; break;
        case Image::FILEFORMAT_PGM:    fiFormat = FIF_PGM; break;
	    case Image::FILEFORMAT_PGMRAW: fiFormat = FIF_PGMRAW; break;
	    case Image::FILEFORMAT_PNG:    fiFormat = FIF_PNG; break;
	    case Image::FILEFORMAT_PPM:    fiFormat = FIF_PPM; break;
	    case Image::FILEFORMAT_PPMRAW: fiFormat = FIF_PPMRAW; break;
	    case Image::FILEFORMAT_TARGA:  fiFormat = FIF_TARGA; break;
	    case Image::FILEFORMAT_TIFF:   fiFormat = FIF_TIFF; break;
	    case Image::FILEFORMAT_XBM:    fiFormat = FIF_XBM; break;
	    case Image::FILEFORMAT_XPM:    fiFormat = FIF_XPM; break;
	    case Image::FILEFORMAT_DDS:    fiFormat = FIF_DDS; break;
	    case Image::FILEFORMAT_GIF:    fiFormat = FIF_GIF; break;
	    case Image::FILEFORMAT_HDR:    fiFormat = FIF_HDR; break;
	    case Image::FILEFORMAT_EXR:    fiFormat = FIF_EXR; break;
	    case Image::FILEFORMAT_RAW:    fiFormat = FIF_RAW; break;
        default:
            debugAssert(static_cast<int>(fileFormat) < Image::FILEFORMAT_MAX);
            fiFormat = static_cast<FREE_IMAGE_FORMAT>(fileFormat);
            break;
    }

    return fiFormat;
}

} // namespace G3D

#endif // dis-allow compilation (while in development)
