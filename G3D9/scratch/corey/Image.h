

#ifndef G3D_IMAGE_H
#define G3D_IMAGE_H

#include "FreeImagePlus.h"
#include "ImageBuffer.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Vector2int32.h"

namespace G3D {

class Image : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Image> Ref;

private:
    fipImage    m_image;

    Image();

public:
    virtual ~Image();

    static Ref fromFile(const std::string& filename);

    void save(const std::string& filename) const;

    void get(const Point2int32& pos, Color3& color) const;
};

} // namespace G3D

#endif // G3D_IMAGE_h