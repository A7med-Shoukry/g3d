/**
  \file G3D/ImageConvert.h

  \created 2012-05-24
  \edited  2012-05-24
*/

#ifndef G3D_ImageConvert_H
#define G3D_ImageConvert_H

#include "G3D/ImageBuffer.h"

namespace G3D {


class ImageConvert {
private:
    ImageConvert();
    
public:
    static ImageBuffer::Ref convertBuffer(const ImageBuffer::Ref& src, const ImageFormat* dstFormat);
};

} // namespace G3D

#endif // GLG3D_ImageConvert_H