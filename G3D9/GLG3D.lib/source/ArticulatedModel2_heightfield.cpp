/**
 \file GLG3D/source/ArticulatedModel2_heightfield.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-10-12
 \edited  2011-10-12
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/FileSystem.h"

namespace G3D {

void ArticulatedModel2::loadHeightfield(const Specification& specification) {
    Part* part = addPart("root");
    Mesh* mesh = addMesh("mesh", part);
    mesh->material = Material::create();
    
    Image1::Ref im = Image1::fromFile(specification.filename);
            
    name = FilePath::base(specification.filename);
    
    part->cpuVertexArray.hasTangent = false;
    part->cpuVertexArray.hasTexCoord0 = true;
    part->m_hasTexCoord0 = true;
    
    const bool spaceCentered = true;
    const bool generateBackFaces = false;

    Array<Point3> vertex;
    Array<Point2> texCoord;
    MeshAlg::generateGrid
        (vertex, texCoord, mesh->cpuIndexArray, 
        im->width(), im->height(),
        Vector2(1, 1),
        spaceCentered,
        generateBackFaces,
        CFrame(Matrix4::scale(im->width(), 1.0, im->height()).upper3x3()),
        im);


    // Copy the vertex data into the mesh
    part->cpuVertexArray.vertex.resize(vertex.size());
    CPUVertexArray::Vertex* vertexPtr = part->cpuVertexArray.vertex.getCArray();
    for (uint32 i = 0; i < (uint32)vertex.size(); ++i) {
        CPUVertexArray::Vertex& v = vertexPtr[i];
        v.position = vertex[i];
        v.texCoord0 = texCoord[i];
        v.tangent.x = v.normal.x = fnan();
    } // for
}

} // namespace G3D
