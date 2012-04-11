

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

#include "G3D/Color4.h"
#include "G3D/Color4unorm8.h"
#include "G3D/Color1unorm8.h"
#include "G3D/ImageBuffer.h"
#include "G3D/Vector2int32.h"
#include "G3D/ReferenceCount.h"


// non-G3D forward declarations
class fipImage;

namespace G3D {

// Forward declarations
class BinaryInput;
class ImageFormat;


class Image : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Image> Ref;

    /// Simply a duplicate of the old GImage::Error right now
    class Error {
    public:
        Error
        (const std::string& reason,
         const std::string& filename = "") :
        reason(reason), filename(filename) {}
        
        std::string reason;
        std::string filename;
    };

private:
    fipImage*           m_image;
    const ImageFormat*  m_format;

    Image();
    Image(const Image&);
    Image& operator=(const Image&);

public:
    virtual ~Image();

    static bool fileSupported(const std::string& filename, bool allowCheckSignature = false);

    static Ref fromFile(const std::string& filename, const ImageFormat* imageFormat = ImageFormat::AUTO());
    static Ref fromBinaryInput(BinaryInput* bi, const ImageFormat* imageFormat = ImageFormat::AUTO());
    static Ref fromBuffer(const ImageBuffer::Ref& buffer);

    void toFile(const std::string& filename) const;
    ImageBuffer::Ref toBuffer() const;

    Ref clone() const;

    int width() const;
    int height() const;
    const ImageFormat* format() const;

    void flipVertical();
    void flipHorizontal();
    void rotateCW(double radians, int numRotations = 1);
    void rotateCCW(double radians, int numRotations = 1);

    /// Direct replacements for old GImage functions for now
    bool convertToL8();
    bool convertToRGB8();
    bool convertToRGBA8();

    void get(const Point2int32& pos, Color4& color) const;
    void get(const Point2int32& pos, Color3& color) const;
    void get(const Point2int32& pos, Color4unorm8& color) const;
    void get(const Point2int32& pos, Color3unorm8& color) const;
    void get(const Point2int32& pos, Color1unorm8& color) const;

    void set(const Point2int32& pos, const Color4& color);
    void set(const Point2int32& pos, const Color3& color);
    void set(const Point2int32& pos, const Color4unorm8& color);
    void set(const Point2int32& pos, const Color3unorm8& color);
    void set(const Point2int32& pos, const Color1unorm8& color);

};

} // namespace G3D

#endif // G3D_IMAGE_h