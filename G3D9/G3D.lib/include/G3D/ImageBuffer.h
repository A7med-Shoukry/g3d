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
    MemoryManager::Ref  m_memoryManager;
    void*               m_buffer;

    const ImageFormat*  m_format;
    int                 m_rowAlignment;
    int                 m_rowStride;

    int                 m_width;
    int                 m_height;
    int                 m_depth;

    ImageBuffer(const ImageFormat* format, int width, int height, int depth, int rowAlignment);

    void allocateBuffer(MemoryManager::Ref memoryManager);
    void freeBuffer();

public:
    typedef ReferenceCountedPointer<ImageBuffer> Ref;

    static Ref create(MemoryManager::Ref memoryManager, const ImageFormat* format, int width, int height, int depth = 1, int rowAlignment = 1);

    ~ImageBuffer();

    const ImageFormat* format() const   { return m_format; }

    int size() const                    { return m_width * m_height * m_depth * m_rowStride; }
    int rowAlignment() const            { return m_rowAlignment; }
    int stride() const                  { return m_rowStride; }

    int width() const                   { return m_width; }
    int height() const                  { return m_height; }
    int depth() const                   { return m_depth; }

    void* buffer()                      { return m_buffer; }
    const void* buffer() const          { return m_buffer; }

    void* row(int y, int d = 0);
    const void* row(int y, int d = 0) const;

};

} // namespace G3D

#endif // G3D_IMAGEBUFFER_H