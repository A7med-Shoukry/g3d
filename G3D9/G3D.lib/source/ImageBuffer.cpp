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


ImageBuffer::ImageBuffer(const ImageFormat* format, int width, int height, int depth, int alignment) 
    : m_buffer(NULL)
    , m_format(format)
    , m_alignment(alignment)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
    , m_memoryManager(NULL) {

    debugAssert(m_format);
    debugAssert(isPow2(m_alignment));
    debugAssert(m_width > 0);
    debugAssert(m_height > 0);
    debugAssert(m_depth > 0);
}

ImageBuffer::Ref ImageBuffer::create(MemoryManager* memoryManager, const ImageFormat* format, int width, int height, int depth, int alignment) {
    ImageBuffer* imageBuffer = new ImageBuffer(format, width, height, depth, alignment);

    // Allocate buffer with memory manager, this reference now owns the buffer
    imageBuffer->allocateBuffer(memoryManager);

    return imageBuffer;
}

ImageBuffer::Ref ImageBuffer::create(void* buffer, const ImageFormat* format, int width, int height, int depth, int alignment) {
    ImageBuffer* imageBuffer = new ImageBuffer(format, width, height, depth, alignment);

    // Set buffer to use, this reference does not own the buffer
    imageBuffer->m_buffer = buffer;

    return imageBuffer;
}

ImageBuffer::~ImageBuffer() {
    if (m_memoryManager) {
        freeBuffer();
    }
}

void ImageBuffer::allocateBuffer(MemoryManager* memoryManager) {
    debugAssert(m_memoryManager == NULL);
    debugAssert(m_buffer == NULL);

    // set memory manager
    m_memoryManager = memoryManager;

    // allocate buffer
    int rowStride = m_width * (m_format->cpuBitsPerPixel / 8);
    rowStride = (rowStride + (m_alignment - 1)) & (~ (m_alignment - 1));

    int bufferSize = m_depth * m_height * m_width * rowStride;
    m_buffer = m_memoryManager->alloc(bufferSize);
}

void ImageBuffer::freeBuffer() {
    debugAssert(m_memoryManager);
    debugAssert(m_buffer);

    m_memoryManager->free(m_buffer);
    m_buffer = NULL;
}

} // namespace G3D