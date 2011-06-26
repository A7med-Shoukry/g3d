/**
 \file glFormat.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-08-07
 \edited  2011-06-19

 Copyright 2002-2011, Morgan McGuire.
 All rights reserved.
*/
#ifndef glFormat_h
#define glFormat_h

#include "glheaders.h"
#include "G3D/g3dmath.h"

// This implementation is designed to meet the following constraints:
//   1. Work around the many MSVC++ partial template bugs
//   2. Work for primitive types (e.g. int)

/** \def glFormatOf

    \brief A macro that maps G3D types to OpenGL formats
    (e.g. <code>glFormat(Vector3) == GL_FLOAT</code>).

    Use <code>DECLARE_GLFORMATOF(MyType, GLType, bool)</code> at top-level to define
    glFormatOf values for your own classes.

    Used by the vertex array infrastructure. */
#define glFormatOf(T) (G3D::_internal::_GLFormat<T>::type())
#define canBeIndexType(T) (G3D::_internal::_GLFormat<T>::canBeIndex())

namespace G3D {
namespace _internal {


template<class T> class _GLFormat {
public:
    static GLenum type() {
        return GL_NONE;
    }
    /** Is this an integer (usable as an index) */
    static bool isInt() {
        return false;
    }
};

}
}

/**
   \def DECLARE_GLFORMATOF

 \brief Macro to declare the underlying format (as will be returned by glFormatOf)
 of a type.  

 For example,

 \code
    DECLARE_GLFORMATOF(Vector4, GL_FLOAT, false)
  \endcode

  Use this so you can make vertex arrays of your own classes and not just 
  the standard ones.

  \param _isIndex True for types that can be used as indices.
 */
#define DECLARE_GLFORMATOF(G3DType, GLType, _isIndex)  \
namespace G3D {                                      \
    namespace _internal {                            \
        template<> class _GLFormat<G3DType> {        \
        public:                                      \
            static GLenum type()  {                  \
                return GLType;                       \
            }                                        \
            static bool canBeIndex() {               \
                return _isIndex;                     \
            }                                        \
        };                                           \
    }                                                \
}

DECLARE_GLFORMATOF( Vector2,       GL_FLOAT,          false)
DECLARE_GLFORMATOF( Vector2int16,  GL_SHORT,          false)
DECLARE_GLFORMATOF( Vector2int32,  GL_INT,            false)

DECLARE_GLFORMATOF( Vector3,       GL_FLOAT,          false)
DECLARE_GLFORMATOF( Vector3int16,  GL_SHORT,          false)
DECLARE_GLFORMATOF( Vector3int32,  GL_INT,            false)

DECLARE_GLFORMATOF( Vector4,       GL_FLOAT,          false)
DECLARE_GLFORMATOF( Vector4int16,  GL_SHORT,          false)
DECLARE_GLFORMATOF( Vector4int8,   GL_BYTE,           false)

DECLARE_GLFORMATOF( Color3uint8,   GL_UNSIGNED_BYTE,  false)
DECLARE_GLFORMATOF( Color3,        GL_FLOAT,          false)
DECLARE_GLFORMATOF( Color4,        GL_FLOAT,          false)
DECLARE_GLFORMATOF( Color4uint8,   GL_UNSIGNED_BYTE,  false)
DECLARE_GLFORMATOF( uint8,         GL_UNSIGNED_BYTE,  true)
DECLARE_GLFORMATOF( uint16,        GL_UNSIGNED_SHORT, true)
DECLARE_GLFORMATOF( uint32,        GL_UNSIGNED_INT,   true)
DECLARE_GLFORMATOF( int8,          GL_BYTE,           true)
DECLARE_GLFORMATOF( int16,         GL_SHORT,          true)
DECLARE_GLFORMATOF( int32,         GL_INT,            true)
DECLARE_GLFORMATOF( float,         GL_FLOAT,          false)
#ifndef G3D_ANDROID
DECLARE_GLFORMATOF( double,        GL_DOUBLE,         false)
#endif

#endif
