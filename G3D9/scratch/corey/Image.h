

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

#include "FreeImagePlus.h"
#include "ImageBuffer.h"
#include "G3D/ReferenceCount.h"
#include "G3D/WrapMode.h"

namespace G3D {

class Image : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Image> Ref;

private:
    fipImage    m_image;

    WrapMode    m_wrapMode;

    Image();

public:
    virtual ~Image();

    static Ref fromFile(const std::string& filename);
    static Ref fromBuffer(ImageBuffer::Ref ImageBuffer);

    static void save(const std::string& filename, ImageBuffer::Ref imageBuffer);
    void save(const std::string& filename) const;

    WrapMode wrapMode() const           { return m_wrapMode; }
    void setWrapMode(WrapMode mode)     { m_wrapMode = mode; }

    void get(float x, float y, Color1& color);
    void get(float x, float y, Color3& color);
    void get(float x, float y, Color4& color);
    void get(float x, float y, Color4uint8& color);

    void set(float x, float y, const Color1& color);
    void set(float x, float y, const Color3& color);
    void set(float x, float y, const Color4& color);
    void set(float x, float y, const Color4uint8 color);
};

} // namespace G3D

#endif // G3D_IMAGE_h