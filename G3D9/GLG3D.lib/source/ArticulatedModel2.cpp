/**
 \file GLG3D/source/ArticulatedModel2.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.


 TODO:
 - Set bump map parallax steps
 - Transform
 - Intersect
 - Load other formats: IFS, PLY2, PLY, 3DS
 - Create heightfield
 - Create cornell box
 - Optimize mergeVertices
 - Optimize parse
 - Pack tangents into short4 format?
 */
#include "GLG3D/ArticulatedModel2.h"

namespace G3D {

const ArticulatedModel2::Pose& ArticulatedModel2::defaultPose() {
    static const Pose p;
    return p;
}


ArticulatedModel2::Ref ArticulatedModel2::create(const ArticulatedModel2::Specification& specification) {
    Ref a = new ArticulatedModel2();
    a->load(specification);
    return a;
}


void ArticulatedModel2::forEachPart(PartCallback& callback, const CFrame& parentFrame, Part* part) {
    // Net transformation from part to world space
    const CFrame& net = parentFrame * part->cframe;

    // Process all children
    for (int c = 0; c < part->m_child.size(); ++c) {
        forEachPart(callback, net, part->m_child[c]);
    }

    // Invoke the callback on this part
    callback(Ref(this), part, parentFrame);
}

void ArticulatedModel2::forEachPart(PartCallback& callback) {
    for (int p = 0; p < m_rootArray.size(); ++p) {
        forEachPart(callback, CFrame(), m_rootArray[p]);
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


class AMTransform : public ArticulatedModel2::PartCallback {
    Matrix4 xform, normalXForm;

public:
    AMTransform(const Matrix4& xform) : xform(xform), normalXForm(xform.upper3x3().inverse().transpose()) {}

    virtual void operator()(ArticulatedModel2::Ref m, ArticulatedModel2::Part* part, const CFrame& parentFrame) override {

        Matrix4 vertexTransform;
        Matrix3 normalTransform;

        if (part->isRoot()) {
            alwaysAssertM(false, "Not implemented");
            // TODO
        } else {
            alwaysAssertM(false, "Not implemented");
            // Don't translate this part
        }

        for (int v = 0; v < part->cpuVertexArray.size(); ++v) {
            CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[v];
            vertex.position = vertexTransform.homoMul(vertex.position, 1.0f);
            vertex.normal   = (normalTransform * vertex.normal).directionOrZero();
        }
    }
}; 


void ArticulatedModel2::load(const Specification& specification) {
    Stopwatch timer;

    if (endsWith(toLower(specification.filename), ".obj")) {
        loadOBJ(specification);
    } else {
        // Error
        throw std::string("Unrecognized file extension on \"") + specification.filename + "\"";
    }
    timer.after("parse file");

    // Perform operations as demanded by the specification
    // TODO: operations
    timer.after("transform");

    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    cleanGeometry(specification.cleanGeometrySettings);
    timer.after("cleanGeometry");
}


bool ArticulatedModel2::intersect
    (const Ray&     R, 
    const CFrame&   cframe, 
    const Pose&     pose, 
    float&          maxDistance, 
    Part*&          part, 
    Mesh*&          mesh, 
    int&            triStartIndex, 
    float&          u, 
    float&          v) const {

    alwaysAssertM(false, "TODO: ArticulatedModel2::intersect");
    return false;
}

} // namespace G3D
