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


ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height, int depth, int rowAlignment) 
    : m_buffer(NULL)
    , m_format(format)
    , m_rowAlignment(rowAlignment)
    , m_rowStride(0)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
    , m_memoryManager(NULL) {

    debugAssert(m_format);
    debugAssert(isPow2(m_rowAlignment));
    debugAssert(m_width > 0);
    debugAssert(m_height > 0);
    debugAssert(m_depth > 0);
}

ImageBuffer::Ref ImageBuffer::create(MemoryManager::Ref memoryManager, const ImageFormat* format, int width, int height, int depth, int rowAlignment) {
    ImageBuffer* imageBuffer = new ImageBuffer(format, width, height, depth, rowAlignment);

    // Allocate buffer with memory manager, this reference now owns the buffer
    imageBuffer->allocateBuffer(memoryManager);

    return imageBuffer;
}

ImageBuffer::~ImageBuffer() {
    if (m_buffer) {
        freeBuffer();
    }
}

void ImageBuffer::allocateBuffer(MemoryManager::Ref memoryManager) {
    debugAssert(m_memoryManager.isNull());
    debugAssert(m_buffer == NULL);

    // set memory manager
    m_memoryManager = memoryManager;

    // allocate buffer
    m_rowStride = m_width * (m_format->cpuBitsPerPixel / 8);
    m_rowStride = (m_rowStride + (m_rowAlignment - 1)) & (~ (m_rowAlignment - 1));

    int bufferSize = m_depth * m_height * m_rowStride;
    m_buffer = m_memoryManager->alloc(bufferSize);
}

void ImageBuffer::freeBuffer() {
    debugAssert(m_memoryManager.notNull());
    debugAssert(m_buffer);

    m_memoryManager->free(m_buffer);
    m_buffer = NULL;
}

void* ImageBuffer::row(int y, int d) {
    debugAssert(y < m_height && d < m_depth);
    return static_cast<uint8*>(m_buffer) + (d * m_height * m_rowStride) + (y * m_rowStride); 
}

const void* ImageBuffer::row(int y, int d) const {
    debugAssert(y < m_height && d < m_depth);
    return static_cast<uint8*>(m_buffer) + (d * m_height * m_rowStride) + (y * m_rowStride); 
}


} // namespace G3D
