/**
 \file GLG3D/source/ArticulatedModel2_PLY.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/ParsePLY.h"
#include "G3D/FileSystem.h"

namespace G3D {

void ArticulatedModel2::loadPLY(const Specification& specification) {

    // Read the data in
    name = FilePath::base(specification.filename);
    Part* part = addPart(name);
    Mesh* mesh = addMesh("mesh", part);
    mesh->material = Material::create();
    
    ParsePLY parseData;
    {
        BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);
        parseData.parse(bi);
    }

    // Convert the format
    
    part->cpuVertexArray.vertex.resize(parseData.numVertices);
    part->cpuVertexArray.hasTangent = false;
    part->cpuVertexArray.hasTexCoord0 = false;
    part->m_hasTexCoord0 = false;
     
    // The PLY format is technically completely flexible, so we have
    // to search for the location of the X, Y, and Z fields within each
    // vertex.
    int axisIndex[3];
    const std::string axisName[3] = {"x", "y", "z"};
    const int numVertexProperties = parseData.vertexProperty.size();
    for (int a = 0; a < 3; ++a) {
        axisIndex[a] = 0;
        for (int p = 0; p < numVertexProperties; ++p) {
            if (parseData.vertexProperty[p].name == axisName[a]) {
                axisIndex[a] = p;
                break;
            }
        }
    }

    for (int v = 0; v < parseData.numVertices; ++v) {
        CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[v];

        // Read the position
        for (int a = 0; a < 3; ++a) {
            vertex.position[a] = parseData.vertexData[v * numVertexProperties + axisIndex[a]];
        }

        // Flag the normal as undefined 
        vertex.normal.x = fnan();
    }


    for (int f = 0; f < parseData.numFaces; ++f) {
        const ParsePLY::Face& face = parseData.faceArray[f];
        
        // Read and tessellate into triangles, assuming convex polygons
        for (int i = 2; i < face.size(); ++i) {
            mesh->cpuIndexArray.append(face[0], face[1], face[i]);
        }
    }
}

} // namespace G3D
