/**
 \file GLG3D/source/ArticulatedModel2_PLY2.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/FileSystem.h"

namespace G3D {

// There is no "ParsePLY2" because PLY2 parsing is trivial--it has no subparts or materials,
// and is directly an indexed format.
void ArticulatedModel2::loadPLY2(const Specification& specification) {
    Part* part = addPart("root");
    Mesh* mesh = addMesh("mesh", part);
    mesh->material = Material::create();
    
    BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);

    if (bi.getLength() == 0) {
        throw std::string("Failed to open " + specification.filename);
    }
        
    const std::string& header = bi.readString32();
    if (strcmp(header.c_str(), "IFS") != 0) {
        throw std::string("File is not an IFS file");
    }

    const float32 ifsversion  = bi.readFloat32();
    if (ifsversion != 1.0f && ifsversion != 1.1f) {
        throw std::string("Bad IFS version, expecting 1.0 or 1.1");
    }
        
    name = bi.readString32();

    part->cpuVertexArray.hasTangent = false;
    part->cpuVertexArray.hasTexCoord0 = false;
    part->m_hasTexCoord0 = false;

    while (bi.hasMore()) {
        std::string str = bi.readString32();
            
        if (str == "VERTICES") {
            debugAssertM(part->cpuVertexArray.size() == 0, "Multiple vertex fields!");
            const uint32 num = bi.readUInt32();
                
            if ((num <= 0) || (num > 10000000)) {
                throw std::string("Bad number of vertices");
            }
            
            part->cpuVertexArray.vertex.resize(num);
               
            CPUVertexArray::Vertex* vertexPtr = part->cpuVertexArray.vertex.getCArray();
            for (uint32 i = 0; i < num; ++i) {
                CPUVertexArray::Vertex& vertex = vertexPtr[i];
                vertex.position.deserialize(bi);
                vertex.tangent.x = vertex.normal.x = fnan();
            }
                
        } else if (str == "TRIANGLES") {
            debugAssertM(mesh->cpuIndexArray.size() == 0,
                            "Multiple triangle fields!");
            const uint32 num = bi.readUInt32();
                
            if ((num <= 0) || (num > 100000000)) {
                throw std::string("Bad number of triangles");
            }
                
            mesh->cpuIndexArray.resize(num * 3);
            for (uint32 i = 0; i < (uint32)mesh->cpuIndexArray.size(); ++i) {
                mesh->cpuIndexArray[i] = bi.readUInt32();
            }
        } else if (str == "TEXTURECOORD") {
            debugAssertM(ifsversion == 1.1f,
                            "IFS Version should be 1.1");
            const uint32 num = bi.readUInt32();
            debugAssertM((int)num == part->cpuVertexArray.size(),
                            " Must have same number of texcoords as vertices");

            part->cpuVertexArray.hasTexCoord0 = true;
            part->m_hasTexCoord0 = true;
            CPUVertexArray::Vertex* vertexPtr = part->cpuVertexArray.vertex.getCArray();
            for(uint32 t = 0; t < num; ++t) {
                vertexPtr[t].texCoord0.deserialize(bi);
            }
        }
    } // while has more data

}

} // namespace G3D
