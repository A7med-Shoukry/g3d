/**
 \file GLG3D/source/ArticulatedModel_PLY2.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel.h"
#include "G3D/FileSystem.h"

namespace G3D {

// There is no "ParsePLY2" because PLY2 parsing is trivial--it has no subparts or materials,
// and is directly an indexed format.
void ArticulatedModel::loadPLY2(const Specification& specification) {
    
    name = FilePath::base(specification.filename);
    Part* part = addPart(name);
    Mesh* mesh = addMesh("mesh", part);
    mesh->material = Material::create();
    
    TextInput ti(specification.filename);
        
    const int nV = iFloor(ti.readNumber());
    const int nF = iFloor(ti.readNumber());
    
    part->cpuVertexArray.vertex.resize(nV);
    mesh->cpuIndexArray.resize(3 * nF);
    part->cpuVertexArray.hasTangent = false;
    part->cpuVertexArray.hasTexCoord0 = false;
    part->m_hasTexCoord0 = false;
        
    for (int i = 0; i < nV; ++i) {
        part->cpuVertexArray.vertex[i].normal = Vector3::nan();
        Vector3& v = part->cpuVertexArray.vertex[i].position;
        for (int a = 0; a < 3; ++a) {
            v[a] = ti.readNumber();
        }
    }
                
    for (int i = 0; i < nF; ++i) {
        const int three = ti.readInteger();
        alwaysAssertM(three == 3, "Ill-formed PLY2 file");
        for (int j = 0; j < 3; ++j) {
            mesh->cpuIndexArray[3*i + j] = ti.readInteger();
        }
    }
}

} // namespace G3D
