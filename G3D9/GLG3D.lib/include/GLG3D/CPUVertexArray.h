/**
 \file GLG3D/CPUVertexArray.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-22
 \edited  2011-07-22
 
 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
*/

#ifndef GLG3D_CPUVertexArray_h
#define GLG3D_CPUVertexArray_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/Vector2unorm16.h"
#include "GLG3D/VertexBuffer.h"

namespace G3D {

class VertexRange;
class CoordinateFrame;

/** \brief Array of vertices with interlaced position, normal, texCoord, and tangent attributes.

\beta 

\sa G3D::Surface, G3D::SuperSurface::CPUGeom, G3D::MeshAlg, G3D::Triangle, G3D::TriTree
*/

class CPUVertexArray {
private:
    //Intentionally unimplemented
    CPUVertexArray& operator=(const CPUVertexArray&); 

public:

    /** \brief Packed vertex attributes. 48 bytes per vertex.
    
    \sa Part::cpuVertexArray */
    class Vertex {
    public:
        /** Part-space position */
        Point3                  position;

        /** Part-space normal */
        Vector3                 normal;

        /** xyz = tangent, w = sign */
        Vector4                 tangent;

        /** Texture coordinate 0. */
        Point2                  texCoord0;

        void transformBy(const CoordinateFrame& cframe);
    };

    Array<Vertex>               vertex;

    /** A second texture coordinate (which is not necessarily stored in
        texture coordinate attribute 1 on a GPU).  This must be on [0,1].
        Typically used for light map coordinates. 
        
        This is stored outside of the CPUVertexArray::vertex array because 
        it is not used by most models. */
    Array<Point2unorm16>        texCoord1;

    /** True if texCoord0 contains valid data. */
    bool                        hasTexCoord0;

    /** True if texCoord1 contains valid data. */
    bool                        hasTexCoord1;

    /** True if tangent contains valid data. */
    bool                        hasTangent;

    CPUVertexArray() : hasTexCoord0(true), hasTexCoord1(false), hasTangent(true) {}

    
    explicit CPUVertexArray(const CPUVertexArray& otherArray);


    void transformAndAppend(const CPUVertexArray& otherArray, const CoordinateFrame& cframe);

    int size() const {
        return vertex.size();
    }

    void copyFrom(const CPUVertexArray& other);

    /** \param texCoord1 This is not interleaved with the other data in GPU memory. 
         Note that G3D::SuperSurface stores this in texture coordinate <b>2</b> because texcoord 1 is used for the tangent. */
    void copyToGPU
    (VertexRange&               vertex, 
     VertexRange&               normal, 
     VertexRange&               packedTangent, 
     VertexRange&               texCoord0,
     VertexRange&               texCoord1,
     VertexBuffer::UsageHint    hint = VertexBuffer::WRITE_ONCE) const;
};

} // namespace G3D

#endif // GLG3D_CPUVertexArray
