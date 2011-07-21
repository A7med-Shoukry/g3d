#include "ArticulatedModel2.h"


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
            part->cpuVertexArray[v] = vertexTransform.homoMul(part->cpuVertexArray[v], 1.0f);
            part->cpuNormalArray[v] = (normalTransform * part->cpuVertexArray[v]).directionOrZero();
        }
    }
}; 


void ArticulatedModel2::load(const Specification& specification) {

    if (endsWith(toLower(specification.filename), ".obj")) {
        loadOBJ(specification);
    } else {
        // Error
        throw std::string("Unrecognized file extension on \"") + specification.filename + "\"";
    }

    //transform as demanded by the specification
    if (specification.xform != Matrix4::identity()) {
        AMTransform transform(specification.xform);
        forEachPart(transform);
    }

    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    cleanGeometry();
}


void ArticulatedModel2::cleanGeometry() {
    for (int p = 0; p < m_partArray.size(); ++p) {
        m_partArray[p]->cleanGeometry();
    }
}


void ArticulatedModel2::Part::cleanGeometry() {
    alwaysAssertM(false, "TODO");

    bool computeSomeTangents = cpuTangentArray.size() == 0;
    bool computeSomeNormals = (cpuNormalArray.size() == 0);

    // See if normals are needed
    if (! computeSomeNormals) {
        // Maybe there is a NaN normal in there
        for (int i = 0; i < cpuNormalArray.size(); ++i) {
            if (isNaN(cpuNormalArray[i].x)) {
                computeSomeNormals = true;

                if (cpuTangentArray.size() > 0) {
                    computeSomeTangents = true;
                    // Wipe out the corresponding tangent vector
                    cpuTangentArray[i].x = fnan();
                }
            }
        }
    }

    const bool hasTexCoords = cpuTexCoord0Array.size() > 0;

    // See if tangents are needed
    if (! computeSomeTangents && hasTexCoords) {
        // Maybe there is a NaN tangent in there
        for (int i = 0; i < cpuTangentArray.size(); ++i) {
            if (isNaN(cpuTangentArray[i].x)) {
                computeSomeTangents = true;
                break;
            }
        }
    }
    
    // TODO...
}
