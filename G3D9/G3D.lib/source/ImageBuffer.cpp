/**
  \file G3D/ImageBuffer.cpp
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2011-08-18
  \edited  2011-08-18

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#include "G3D/ImageBuffer.h"


namespace G3D {


ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height, int stride, MemoryManager* memoryManager) 
    : m_buffer(NULL)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_memoryManager(memoryManager) {

    debugAssert(m_format);
    debugAssert(m_width > 0);
    debugAssert(m_height > 0);
    debugAssert(m_stride >= (m_width * m_format->cpuBitsPerPixel / 8));
    debugAssert(m_memoryManager);

    // allocate buffer
    m_buffer = m_memoryManager->alloc(m_stride * height);
}

ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height, int stride, void* buffer)
    : m_buffer(buffer)
    , m_format(format)
    , m_width(width)
    , m_height(height)
    , m_stride(stride)
    , m_memoryManager(NULL) {

    debugAssert(m_buffer);
    debugAssert(m_format);
    debugAssert(m_width > 0);
    debugAssert(m_height > 0);
    debugAssert(m_stride >= (m_width * m_format->cpuBitsPerPixel / 8));
}

ImageBuffer::~ImageBuffer() {
    if (m_memoryManager) {
        // free buffer
        m_memoryManager->free(m_buffer);
        m_buffer = NULL;
    }
}

} // namespace G3D