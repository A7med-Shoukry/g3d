/**
 \file GLG3D/source/ArticulatedModel.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.


 TODO:
 - Create heightfield
 - Create Cornell Box
 - Remove IFSModel
 - Set bump map parallax steps in specification
 - Implement other preprocess instructions
 - Remove ArticulatedModel
 - Pack tangents and normals into short4 format?
 */
#include "GLG3D/ArticulatedModel.h"
#include "G3D/Ray.h"
#include "G3D/FileSystem.h"

namespace G3D {

const CFrame ArticulatedModel::Pose::identity;

ArticulatedModel::Ref ArticulatedModel::create(const ArticulatedModel::Specification& specification) {
    Ref a = new ArticulatedModel();
    a->load(specification);
    return a;
}


void ArticulatedModel::forEachPart(PartCallback& callback, Part* part, const CFrame& parentFrame, const Pose& pose, const int treeDepth) {
    // Net transformation from part to world space
    const CFrame& net = parentFrame * part->cframe * pose[part->name];

    // Process all children
    for (int c = 0; c < part->m_child.size(); ++c) {
        forEachPart(callback, part->m_child[c], net, pose, treeDepth + 1);
    }

    // Invoke the callback on this part
    callback(part, net, Ref(this), treeDepth);
}


void ArticulatedModel::forEachPart(PartCallback& callback, const CFrame& cframe, const Pose& pose) {
    for (int p = 0; p < m_rootArray.size(); ++p) {
        forEachPart(callback, m_rootArray[p], cframe, pose, 0);
    }
}


ArticulatedModel::Mesh* ArticulatedModel::addMesh(const std::string& name, Part* part) {
    part->m_meshArray.append(new Mesh(name, createID()));
    return part->m_meshArray.last();
}


ArticulatedModel::Part* ArticulatedModel::addPart(const std::string& name, Part* parent) {
    m_partArray.append(new Part(name, parent, createID()));
    if (parent == NULL) {
        m_rootArray.append(m_partArray.last());
    }

    return m_partArray.last();
}


ArticulatedModel::Mesh* ArticulatedModel::mesh(const ID& id) {
    Mesh** ptr = m_meshTable.getPointer(id);
    if (ptr == NULL) {
        return NULL;
    } else {
        return *ptr;
    }
}


ArticulatedModel::Part* ArticulatedModel::part(const ID& id) {
    Part** ptr = m_partTable.getPointer(id);
    if (ptr == NULL) {
        return NULL;
    } else {
        return *ptr;
    }
}


ArticulatedModel::Mesh* ArticulatedModel::mesh(const std::string& partName, const std::string& meshName) {
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


ArticulatedModel::Part* ArticulatedModel::part(const Instruction::Identifier& partIdent) {
    if (partIdent.id.initialized()) {
        return part(partIdent.id);
    } else {
        return part(partIdent.name);
    }
}


ArticulatedModel::Mesh* ArticulatedModel::mesh(const Instruction::Identifier& partIdent, const Instruction::Identifier& meshIdent) {
    if (meshIdent.id.initialized()) {
        return mesh(meshIdent.id);
    }

    Part* partPtr = part(partIdent);
    
    if (partPtr != NULL) {
        for (int i = 0; i < partPtr->m_meshArray.size(); ++i) {
            if (partPtr->m_meshArray[i]->name == meshIdent.name) {
                return partPtr->m_meshArray[i];
            }
        }
    }

    return NULL;
}


ArticulatedModel::Part* ArticulatedModel::part(const std::string& partName) {
    // Exhaustively cycle through all parts
    for (int p = 0; p < m_partArray.size(); ++p) {
        if (m_partArray[p]->name == partName) {
            return m_partArray[p];
        }
    }
    return NULL;
}



void ArticulatedModel::load(const Specification& specification) {
    Stopwatch timer;
    
    const std::string& ext = toLower(FilePath::ext(specification.filename));

    if (ext == "obj") {
        loadOBJ(specification);
    } else if (ext == "ifs") {
        loadIFS(specification);
    } else if (ext == "ply2") {
        loadPLY2(specification);
    } else if (ext == "ply") {
        loadPLY(specification);
    } else if (ext == "off") {
        loadOFF(specification);
    } else if (ext == "3ds") {
        load3DS(specification);
    } else if (ext == "bsp") {
        loadBSP(specification);
    } else if (GImage::supportedFormat(ext)) {
        loadHeightfield(specification);
    } else {
        // Error
        throw std::string("Unrecognized file extension on \"") + specification.filename + "\"";
    }
    timer.after("parse file");

    // If this model is very large, compact the vertex arrays to save RAM
    // during the post-processing step
    maybeCompactArrays();

    // Perform operations as demanded by the specification
    if (specification.scale != 1.0f) {
        ScaleTransformCallback transform(specification.scale);
        forEachPart(transform);
    }
    preprocess(specification.preprocess);
    timer.after("preprocess");

    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    cleanGeometry(specification.cleanGeometrySettings);
    maybeCompactArrays();
    timer.after("cleanGeometry");
}


void ArticulatedModel::maybeCompactArrays() {
    int numVertices = 0;
    int numIndices = 0;

    for (int p = 0; p < m_partArray.size(); ++p) {
        Part* part = m_partArray[p];
        numVertices += part->cpuVertexArray.size();
        for (int m = 0; m < part->m_meshArray.size(); ++m) {
            numIndices += part->m_meshArray[m]->cpuIndexArray.size();
        }
    }
    
    size_t vertexBytes = sizeof(CPUVertexArray::Vertex) * numVertices;
    size_t indexBytes = sizeof(int) * numIndices;

    if (vertexBytes + indexBytes > 5000000) {
        // There's a lot of data in this model: compact it

        for (int p = 0; p < m_partArray.size(); ++p) {
            Part* part = m_partArray[p];
            part->cpuVertexArray.vertex.trimToSize();
            for (int m = 0; m < part->m_meshArray.size(); ++m) {
                part->m_meshArray[m]->cpuIndexArray.trimToSize();
            }
        }
    }
}


/** Used by ArticulatedModel::intersect */
class AMIntersector : public ArticulatedModel::PartCallback {
public:
    bool                        hit;
    const Ray&                  wsR;
    float&                      maxDistance;
    ArticulatedModel::Part*&   partHit;
    ArticulatedModel::Mesh*&   meshHit; 
    int&                        triStartIndex;
    float&                      u;
    float&                      v;

    AMIntersector(const Ray& wsR, float& maxDistance, ArticulatedModel::Part*& partHit, 
        ArticulatedModel::Mesh*& meshHit, int& triStartIndex, float& u, float& v) :
        hit(false),
        wsR(wsR), maxDistance(maxDistance), partHit(partHit), meshHit(meshHit),
        triStartIndex(triStartIndex), u(u), v(v) {
    }

    virtual void operator()(ArticulatedModel::Part* part, const CFrame& partFrame,
                            ArticulatedModel::Ref model, const int treeDepth) override {

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
            ArticulatedModel::Mesh* mesh = part->meshArray()[m];

            const float testTime = osRay.intersectionTime(mesh->boxBounds);
            if (testTime >= maxDistance) {
                // Could not possibly hit this mesh's geometry since it doesn't
                // hit the bounds
                continue;
            }

            const int numIndices = mesh->cpuIndexArray.size();
            const int* index = mesh->cpuIndexArray.getCArray();

            alwaysAssertM(mesh->primitive == PrimitiveType::TRIANGLES, 
                          "Only implemented for PrimitiveType::TRIANGLES meshes.");

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

                if (mesh->twoSided) {
                    // Check the backface
                    float w0 = 0, w1 = 0, w2 = 0;
                    const float testTime = osRay.intersectionTime
                        (vertex[index[i]].position, 
                         vertex[index[i + 2]].position,
                         vertex[index[i + 1]].position,
                         w0, w1, w2);
                    
                    if (testTime < maxDistance) {
                        hit         = true;
                        maxDistance = testTime;
                        partHit     = part;
                        meshHit     = mesh;
                        triStartIndex = i;
                        u           = w0;
                        v           = w2;
                    }
                }
            } // for each triangle
        } // for each mesh
    } // operator ()
}; // AMIntersector


bool ArticulatedModel::intersect
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


void ArticulatedModel::countTrianglesAndVertices(int& tri, int& vert) const {
    tri = 0;
    vert = 0;
    for (int p = 0; p < m_partArray.size(); ++p) {
        const Part* part = m_partArray[p];
        tri += part->triangleCount();
        vert += part->cpuVertexArray.size();
    }
}

} // namespace G3D
