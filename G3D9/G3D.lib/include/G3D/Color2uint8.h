/** 
  \file G3D/Color2uint8.h
 
  \maintainer Morgan McGuire
 
  \created 2011-06-23
  \edited  2011-01-23

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_Color2uint8_h
#define G3D_Color2uint8_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {

/** Matches OpenGL GL_RG8 format.  

Can be used to accessdata in GL_RA8 or GL_LA8 format as well.
*/
G3D_BEGIN_PACKED_CLASS(2)
Color2uint8 {
private:
    // Hidden operators
    bool operator<(const Color2uint8&) const;
    bool operator>(const Color2uint8&) const;
    bool operator<=(const Color2uint8&) const;
    bool operator>=(const Color2uint8&) const;

public:

    uint8       r;
    uint8       g;

    Color2uint8() : r(0), g(0) {}

    explicit Color2uint8(const uint8 _r, const uint8 _g) : r(_r), g(_g) {}

    inline bool operator==(const Color2uint8& other) const {
        return r == other.r && g == other.g;
    }

    inline bool operator!=(const Color2uint8& other) const {
        return r != other.r || g != other.g;
    }

}
G3D_END_PACKED_CLASS(2)
}
#endif
