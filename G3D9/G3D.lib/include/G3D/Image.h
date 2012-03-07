

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

// Dis-allow compilation by default due to dependency on freeImage (while in development)
#if 0

#include "FreeImagePlus.h" // todo: create file format enum in Image to remove this dependency in header
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

private:
    fipImage*           m_image;
    const ImageFormat*  m_format;


    Image();

    const ImageFormat* determineImageFormat() const;

public:
    virtual ~Image();

    static Ref fromFile(const std::string& filename, FREE_IMAGE_FORMAT fileFormat = FIF_UNKNOWN, const ImageFormat* imageFormat = ImageFormat::AUTO());
    static Ref fromInput(const BinaryInput& bi, FREE_IMAGE_FORMAT fileFormat = FIF_UNKNOWN, const ImageFormat* imageFormat = ImageFormat::AUTO());


    void get(const Point2int32& pos, Color4& color) const;
    void get(const Point2int32& pos, Color3& color) const;
    void get(const Point2int32& pos, Color4unorm8& color) const;
    void get(const Point2int32& pos, Color3unorm8& color) const;

    void set(const Point2int32& pos, const Color4& color);
    void set(const Point2int32& pos, const Color3& color);
    void set(const Point2int32& pos, const Color4unorm8& color);
    void set(const Point2int32& pos, const Color3unorm8& color);

    ImageBuffer::Ref copyBuffer();
};

} // namespace G3D

#endif // dis-allow compilation (while in development)

#endif // G3D_IMAGE_h