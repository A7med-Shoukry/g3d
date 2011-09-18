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
    int                 m_alignment;

    int                 m_width;
    int                 m_height;
    int                 m_depth;

    MemoryManager*      m_memoryManager;

    ImageBuffer(const ImageFormat* format, int width, int height, int depth, int alignment);

    void allocateBuffer(MemoryManager* memoryManager);
    void freeBuffer();

public:
    typedef ReferenceCountedPointer<ImageBuffer> Ref;

    static Ref create(MemoryManager* memoryManager, const ImageFormat* format, int width, int height, int depth = 1, int alignment = 1);
    static Ref create(void* buffer, const ImageFormat* format, int width, int height, int depth = 1, int alignment = 1);

    ~ImageBuffer();

    const ImageFormat* format() const   { return m_format; }
    int alignment() const               { return m_alignment; }

    int width() const                   { return m_width; }
    int height() const                  { return m_height; }
    int depth() const                   { return m_depth; }

    void* buffer()                      { return m_buffer; }
    const void* buffer() const          { return m_buffer; }

    void* row(int y);
    const void* row(int y) const;

};

} // namespace G3D

#endif // G3D_IMAGEBUFFER_H