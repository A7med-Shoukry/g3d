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
            ArticulatedModel2::Part::Vertex& vertex = part->cpuVertexArray[v];
            vertex.position = vertexTransform.homoMul(vertex.position, 1.0f);
            vertex.normal   = (normalTransform * vertex.normal).directionOrZero();
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


void ArticulatedModel2::Part::clearVertexRanges() {
    gpuPositionArray      = VertexRange();
    gpuNormalArray        = VertexRange();
    gpuTexCoord0Array     = VertexRange();
    gpuTangentArray       = VertexRange();

    for (int m = 0; m < m_meshArray.size(); ++m) {
        m_meshArray[m]->gpuIndexArray = VertexRange();
    }
}


void ArticulatedModel2::Part::determineCleaningNeeds(bool& computeSomeNormals, bool& computeSomeTangents) {
    computeSomeTangents = false;
    computeSomeNormals = false;;

    // See if normals are needed
    for (int i = 0; i < cpuVertexArray.size(); ++i) {
        if (isNaN(cpuVertexArray[i].normal.x)) {
            computeSomeNormals = true;
            computeSomeTangents = true;
            // Wipe out the corresponding tangent vector
            cpuVertexArray[i].tangent.x = fnan();
        }
    }
    
    // See if tangents are needed
    if (m_hasTexCoord0 && ! computeSomeTangents) {
        // Maybe there is a NaN tangent in there
        for (int i = 0; i < cpuVertexArray.size(); ++i) {
            if (isNaN(cpuVertexArray[i].tangent.x)) {
                computeSomeTangents = true;
                break;
            }
        }
    }
    
    if (m_hasTexCoord0) {
        // If we have no texture coordinates, we are unable to compute tangents.
        computeSomeTangents = false;
    }
}


void ArticulatedModel2::Part::cleanGeometry() {
    clearVertexRanges();

    bool computeSomeTangents = false, computeSomeNormals = false;
    determineCleaningNeeds(computeSomeNormals, computeSomeTangents);

    m_triangleCount = 0;
    for (int m = 0; m < m_meshArray.size(); ++m) {
        alwaysAssertM(m_meshArray[m]->primitive == PrimitiveType::TRIANGLES, 
            "Only implemented for PrimitiveType::TRIANGLES");
        m_triangleCount += m_meshArray[m]->cpuIndexArray.size() / 3;
    }

    if (computeSomeTangents || computeSomeNormals) {
        // Expand into an un-indexed triangle list
        Array<Face> faceArray;
        faceArray.reserve(m_triangleCount);

        for (int m = 0; m < m_meshArray.size(); ++m) {
            const Mesh* mesh = m_meshArray[m];
            const Array<int>& indexArray = mesh->cpuIndexArray;

            // For every indexed triangle
            for (int i = 0; i < indexArray.size(); i += 3) {
                Face& face = faceArray.next();
                for (int v = 0; v < 3; ++v) {
                    int index = indexArray[i + v];
                    Vertex& vertex = face.vertex[v] = cpuVertexArray[index];
                }
            }
        }

        // For each vertex that requires a normal:
        //    Average face normals that are not too far away from this one

        // Merge vertices that have nearly equal normals, positions, and texcoords

//        static void 	computeNormals (const Array< Vector3 > &vertexArray, const Array< Face > &faceArray, const Array< Array< int > > &adjacentFaceArray, Array< Vector3 > &vertexNormalArray, Array< Vector3 > &faceNormalArray)
//        static void 	computeTangentSpaceBasis (const Array< Vector3 > &vertexArray, const Array< Vector2 > &texCoordArray, const Array< Vector3 > &vertexNormalArray, const Array< Face > &faceArray, Array< Vector3 > &tangent, Array< Vector3 > &binormal)
//        collapseSharedVertices();
        alwaysAssertM(false, "TODO");
    }
}
