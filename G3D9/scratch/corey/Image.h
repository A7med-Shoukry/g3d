

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

#include "FreeImagePlus.h"
#include "G3D/referencecount.h"

namespace G3D {

class Image : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Image> Ref;

private:
    fipImage    mImage;

    Image();

public:
    virtual ~Image();

    static Ref fromFile(const std::string& filename);

};

} // namespace G3D

#endif // G3D_IMAGE_h