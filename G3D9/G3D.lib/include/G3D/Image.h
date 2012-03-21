

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

// Dis-allow compilation by default due to dependency on freeImage (while in development)
#if 0

#include "G3D/Color4.h"
#include "G3D/Color4unorm8.h"
#include "G3D/ImageBuffer.h"
#include "G3D/Vector2int32.h"
#include "G3D/ReferenceCount.h"


// non-G3D forward declarations
class fipImage;

namespace G3D {

// Forward declarations
class ImageFormat;


class Image : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Image> Ref;

    /// Direct mapping of FreeImage FREE_IMAGE_FORMAT enum for common formats. Uncommon values can be used directly.
    enum FileFormat {
        FILEFORMAT_AUTO,

        FILEFORMAT_BMP,
        FILEFORMAT_ICO,
        FILEFORMAT_JPEG,
        FILEFORMAT_MNG,
        FILEFORMAT_PBM,
        FILEFORMAT_PBMRAW,
        FILEFORMAT_PCX,
        FILEFORMAT_PGM,
        FILEFORMAT_PGMRAW,
        FILEFORMAT_PNG,
        FILEFORMAT_PPM,
        FILEFORMAT_PPMRAW,
        FILEFORMAT_TARGA,
        FILEFORMAT_TIFF,
        FILEFORMAT_XBM,
        FILEFORMAT_XPM,
        FILEFORMAT_DDS,
        FILEFORMAT_GIF,
        FILEFORMAT_HDR,
        FILEFORMAT_EXR,
        FILEFORMAT_RAW,

        FILEFORMAT_MAX = 256
    };

private:
    fipImage*           m_image;
    const ImageFormat*  m_format;

    Image();
    Image(const Image&);
    Image& operator=(const Image&);

    const ImageFormat* determineImageFormat() const;

public:
    virtual ~Image();

    static Ref fromFile(const std::string& filename, FileFormat fileFormat = Image::FILEFORMAT_AUTO, const ImageFormat* imageFormat = ImageFormat::AUTO());
    static Ref fromBuffer(const ImageBuffer::Ref& buffer);

    void toFile(const std::string& filename) const;
    ImageBuffer::Ref toBuffer() const;

    void get(const Point2int32& pos, Color4& color) const;
    void get(const Point2int32& pos, Color3& color) const;
    void get(const Point2int32& pos, Color4unorm8& color) const;
    void get(const Point2int32& pos, Color3unorm8& color) const;

    void set(const Point2int32& pos, const Color4& color);
    void set(const Point2int32& pos, const Color3& color);
    void set(const Point2int32& pos, const Color4unorm8& color);
    void set(const Point2int32& pos, const Color3unorm8& color);

};

} // namespace G3D

#endif // dis-allow compilation (while in development)

#endif // G3D_IMAGE_h