
#include "ImageBuffer.h"


namespace G3D {


ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height) 
    : m_buffer(NULL)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_ownsBuffer(true) {

    // allocate buffer
}

ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height, void* buffer)
    : m_buffer(buffer)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_ownsBuffer(false) {

    debugAssert(m_buffer);
}

ImageBuffer::~ImageBuffer() {
    if (m_ownsBuffer) {
        // free buffer
        m_buffer = NULL;
    }
}

} // namespace G3D