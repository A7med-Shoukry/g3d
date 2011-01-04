/**
  \file G3D/GUniqueID.h
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef G3D_GUniqueID_h
#define G3D_GUniqueID_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Table.h"
#include "G3D/Any.h"

namespace G3D {

/** Globally unique identifiers. The probability of two different
    programs generating the same value from UniqueID::create is
    vanishingly small.
    
    UniqueIDs optionally contain a 10-bit application specific tag
    that distinguishes their type.
*/
class GUniqueID {
private:

    uint64 id;

public:

    GUniqueID() : id(0) {}

    GUniqueID& operator=(const Any& a) {
        a.verifyName("GUniqueID");
        a.verifyType(Any::ARRAY);
        a.verifySize(4);
        id = (uint64(a[3].number()) << 48) | (uint64(a[2].number()) << 32) | (uint64(a[1].number()) << 16) | uint64(a[0].number());
        return *this;
    }

    GUniqueID(const Any& a) {
        *this = a;
    }

    Any toAny() const {
        Any a(Any::ARRAY, "GUniqueID");
        a.resize(4);
        for (int i = 0; i < 4; ++i) {
            a[i] = Any(float(uint16(id >> (i * 16))));
        }
        return a;
    }

    bool uninitialized() const {
        return id == 0;
    }

    uint16 tag() const {
        return id >> 54;
    }

    operator uint64() const {
        return id;
    }    

    bool operator==(const GUniqueID& other) const {
        return id == other.id;
    }

    bool operator!=(const GUniqueID& other) const {
        return id != other.id;
    }

    void serialize(class BinaryOutput& b) const;

    void deserialize(class BinaryInput& b);

    void serialize(class TextOutput& t) const;

    void deserialize(class TextInput& t);

    /** Create a new ID */
    static GUniqueID create(uint16 tag = 0);
};

} // G3D 

/** For Table and Set */
template<> struct HashTrait<class G3D::GUniqueID> {
    static size_t hashCode(G3D::GUniqueID id) { return (size_t)(G3D::uint64)id; }
};

#endif
