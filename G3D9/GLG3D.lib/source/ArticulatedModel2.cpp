/**
 \file GLG3D/source/ArticulatedModel2.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.


 TODO:
 - implement setMaterial instruction
 - implement setPartCFrame instruction
 - ParsePLY
 - Remove IFSModel
 - Parse3DS
 - Create heightfield
 - Create Cornell Box
 - Import Quake BSP
 - Set bump map parallax steps in specification
 - Implement other preprocess instructions
 - Remove ArticulatedModel?!
 - Multithread by part
 - Pack tangents into short4 format?
 */
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/Ray.h"
#include "G3D/FileSystem.h"

namespace G3D {

const CFrame ArticulatedModel2::Pose::identity;

ArticulatedModel2::Ref ArticulatedModel2::create(const ArticulatedModel2::Specification& specification) {
    Ref a = new ArticulatedModel2();
    a->load(specification);
    return a;
}


void ArticulatedModel2::forEachPart(PartCallback& callback, Part* part, const CFrame& parentFrame, const Pose& pose) {
    // Net transformation from part to world space
    const CFrame& net = parentFrame * part->cframe * pose[part->name];

    // Process all children
    for (int c = 0; c < part->m_child.size(); ++c) {
        forEachPart(callback, part->m_child[c], net, pose);
    }

    // Invoke the callback on this part
    callback(part, parentFrame, Ref(this));
}


void ArticulatedModel2::forEachPart(PartCallback& callback, const CFrame& cframe, const Pose& pose) {
    for (int p = 0; p < m_rootArray.size(); ++p) {
        forEachPart(callback, m_rootArray[p], cframe, pose);
    }
}


ArticulatedModel2::Mesh* ArticulatedModel2::addMesh(const std::string& name, Part* part) {
    part->m_meshArray.append(new Mesh(name, ID(m_nextID)));
    ++m_nextID;
    return part->m_meshArray.last();
}


ArticulatedModel2::Part* ArticulatedModel2::addPart(const std::string& name, Part* parent) {
    m_partArray.append(new Part(name, parent, ID(m_nextID)));
    ++m_nextID;
    if (parent == NULL) {
        m_rootArray.append(m_partArray.last());
    }

    return m_partArray.last();
}


ArticulatedModel2::Mesh* ArticulatedModel2::mesh(const ID& id) {
    Mesh** ptr = m_meshTable.getPointer(id);
    if (ptr == NULL) {
        return NULL;
    } else {
        return *ptr;
    }
}


ArticulatedModel2::Part* ArticulatedModel2::part(const ID& id) {
    Part** ptr = m_partTable.getPointer(id);
    if (ptr == NULL) {
        return NULL;
    } else {
        return *ptr;
    }
}


ArticulatedModel2::Mesh* ArticulatedModel2::mesh(const std::string& partName, const std::string& meshName) {
    Part* p = part(partName);
    if (p != NULL) {
        // Exhaustively cycle through all meshes
        for (int m = 0; m < p->m_meshArray.size(); ++m) {
            if (p->m_meshArray[m]->name == meshName) {
                return p->m_meshArray[m];
            }
        }
    }
    return NULL;
}


ArticulatedModel2::Part* ArticulatedModel2::part(const std::string& partName) {
    // Exhaustively cycle through all parts
    for (int p = 0; p < m_partArray.size(); ++p) {
        if (m_partArray[p]->name == partName) {
            return m_partArray[p];
        }
    }
    return NULL;
}



void ArticulatedModel2::load(const Specification& specification) {
    Stopwatch timer;
    
    const std::string& ext = toLower(FilePath::ext(specification.filename));

    if (ext == "obj") {
        loadOBJ(specification);
    } else if (ext == "ifs") {
        loadIFS(specification);
    } else if (ext == "ply2") {
        loadPLY2(specification);
    } else if (ext == "off") {
        loadOFF(specification);
    } else {
        // Error
        throw std::string("Unrecognized file extension on \"") + specification.filename + "\"";
    }
    timer.after("parse file");

    // Perform operations as demanded by the specification
    preprocess(specification.preprocess);
    timer.after("preprocess");

    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    cleanGeometry(specification.cleanGeometrySettings);
    timer.after("cleanGeometry");
}


/** Used by ArticulatedModel2::intersect */
class AMIntersector : public ArticulatedModel2::PartCallback {
public:
    bool                        hit;
    const Ray&                  wsR;
    float&                      maxDistance;
    ArticulatedModel2::Part*&   partHit;
    ArticulatedModel2::Mesh*&   meshHit; 
    int&                        triStartIndex;
    float&                      u;
    float&                      v;

    AMIntersector(const Ray& wsR, float& maxDistance, ArticulatedModel2::Part*& partHit, 
        ArticulatedModel2::Mesh*& meshHit, int& triStartIndex, float& u, float& v) :
        hit(false),
        wsR(wsR), maxDistance(maxDistance), partHit(partHit), meshHit(meshHit),
        triStartIndex(triStartIndex), u(u), v(v) {
    }

    virtual void operator()(ArticulatedModel2::Part* part, const CFrame& partFrame, ArticulatedModel2::Ref model) override {
        // Take the ray to object space
        const Ray& osRay = partFrame.toObjectSpace(wsR);

        // Bounding sphere test
        const float testTime = osRay.intersectionTime(part->boxBounds);
        if (testTime >= maxDistance) {
            // Could not possibly hit this part's geometry since it doesn't
            // hit the bounds
            return;
        }

        // For each mesh
        const CPUVertexArray::Vertex* vertex = part->cpuVertexArray.vertex.getCArray();
        for (int m = 0; m < part->meshArray().size(); ++m) {
            ArticulatedModel2::Mesh* mesh = part->meshArray()[m];

            const float testTime = osRay.intersectionTime(mesh->boxBounds);
            if (testTime >= maxDistance) {
                // Could not possibly hit this mesh's geometry since it doesn't
                // hit the bounds
                continue;
            }

            const int numIndices = mesh->cpuIndexArray.size();
            const int* index = mesh->cpuIndexArray.getCArray();

            alwaysAssertM(mesh->primitive == PrimitiveType::TRIANGLES, "Only implemented for PrimitiveType::TRIANGLES meshes.");

            for (int i = 0; i < numIndices; i += 3) {    

                // Barycentric weights
                float w0 = 0, w1 = 0, w2 = 0;
                const float testTime = osRay.intersectionTime
                    (vertex[index[i]].position, 
                        vertex[index[i + 1]].position,
                        vertex[index[i + 2]].position,
                        w0, w1, w2);

                if (testTime < maxDistance) {
                    hit         = true;
                    maxDistance = testTime;
                    partHit     = part;
                    meshHit     = mesh;
                    triStartIndex = i;
                    u           = w0;
                    v           = w1;
                }
            } // for each triangle
        } // for each mesh
    } // operator ()
}; // AMIntersector


bool ArticulatedModel2::intersect
    (const Ray&     R, 
    const CFrame&   cframe, 
    const Pose&     pose, 
    float&          maxDistance, 
    Part*&          part, 
    Mesh*&          mesh, 
    int&            triStartIndex, 
    float&          u, 
    float&          v) {

    AMIntersector intersectOperation(R, maxDistance, part, mesh, triStartIndex, u, v);
    forEachPart(intersectOperation, cframe, pose);

    return intersectOperation.hit;
}


void ArticulatedModel2::countTrianglesAndVertices(int& tri, int& vert) const {
    tri = 0;
    vert = 0;
    for (int p = 0; p < m_partArray.size(); ++p) {
        const Part* part = m_partArray[p];
        tri += part->triangleCount();
        vert += part->cpuVertexArray.size();
    }
}

} // namespace G3D
