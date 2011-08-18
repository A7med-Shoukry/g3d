/**
  \file G3D/ImageBuffer.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2011-08-18
  \edited  2011-08-18

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_IMAGEBUFFER_H
#define G3D_IMAGEBUFFER_H

#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"


namespace G3D {

class ImageBuffer : public ReferenceCountedObject {
private:
    void*               m_buffer;
    const ImageFormat*  m_format;

    int                 m_width;
    int                 m_height;
    int                 m_stride;

    MemoryManager*      m_memoryManager;

public:
    typedef ReferenceCountedPointer<ImageBuffer> Ref;

    ImageBuffer(const ImageFormat* format, int width, int height, int stride, MemoryManager* memoryManager);
    ImageBuffer(const ImageFormat* format, int width, int height, int stride, void* buffer);

    ~ImageBuffer();

    const ImageFormat* format() const   { return m_format; }

    int width() const                   { return m_width; }
    int height() const                  { return m_height; }
    int stride() const                  { return m_stride; }

    void* buffer()                      { return m_buffer; }
    const void* buffer() const          { return m_buffer; }

    bool ownsAllocation() const         { return m_memoryManager != NULL; }
};

} // namespace G3D

#endif // G3D_IMAGEBUFFER_H