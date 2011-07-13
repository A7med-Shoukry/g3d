
#ifndef G3D_IMAGEBUFFER_H
#define G3D_IMAGEBUFFER_H

#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"


namespace G3D {

class ImageBuffer : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<ImageBuffer> Ref;

private:
    void*               m_buffer;
    const ImageFormat*  m_format;

    int                 m_width;
    int                 m_height;

    bool                m_ownsBuffer;

public:
    ImageBuffer(const ImageFormat* format, int width, int height);
    ImageBuffer(const ImageFormat* format, int width, int height, void* buffer);

    ~ImageBuffer();

    const ImageFormat* format() const   { return m_format; }

    int width() const                   { return m_width; }
    int height() const                  { return m_height; }

    void* buffer()                      { return m_buffer; }
    const void* buffer() const          { return m_buffer; }

    bool ownsBuffer() const             { return m_ownsBuffer; }
};

} // namespace G3D

#endif // G3D_IMAGEBUFFER_H