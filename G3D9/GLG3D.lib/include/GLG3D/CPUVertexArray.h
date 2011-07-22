/**
 \file GLG3D/CPUVertexArray.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-22
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/

#ifndef GLG3D_CPUVertexArray_h
#define GLG3D_CPUVertexArray_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "GLG3D/VertexBuffer.h"

namespace G3D {

class VertexRange;

/** \brief Array of vertices with interlaced position, normal, texCoord, and tangent attributes.

\beta 

\sa G3D::Surface, G3D::SuperSurface::CPUGeom, G3D::MeshAlg, G3D::Triangle, G3D::TriTree
*/
class CPUVertexArray {
public:

    /** \brief Packed vertex attributes. 
    
    \sa Part::cpuVertexArray */
    class Vertex {
    public:
        /** Part-space position */
        Point3                  position;

        /** Part-space normal */
        Vector3                 normal;

        /** xyz = tangent, w = sign */
        Vector4                 tangent;

        /** Texture coordinate 0, setting a convention for expansion in later API versions. */
        Point2                  texCoord0;
    };

    Array<Vertex>               vertex;

    /** True if texCoord0 contains valid data. */
    bool                        hasTexCoord0;

    /** True if tangent contains valid data. */
    bool                        hasTangent;

    CPUVertexArray() : hasTexCoord0(true), hasTangent(true) {}

    int size() const {
        return vertex.size();
    }

    void copyToGPU
    (VertexRange&               vertex, 
     VertexRange&               normal, 
     VertexRange&               packedTangent, 
     VertexRange&               texCoord0,
     VertexBuffer::UsageHint    hint = VertexBuffer::WRITE_ONCE) const;
};

} // namespace G3D

#endif // GLG3D_CPUVertexArray
