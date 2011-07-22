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
    cleanGeometry(true);
}


void ArticulatedModel2::cleanGeometry(bool alwaysMergeVertices) {
    for (int p = 0; p < m_partArray.size(); ++p) {
        m_partArray[p]->cleanGeometry(alwaysMergeVertices);
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


void ArticulatedModel2::Part::cleanGeometry(bool alwaysMergeVertices) {
    clearVertexRanges();

    bool computeSomeTangents = false, computeSomeNormals = false;
    determineCleaningNeeds(computeSomeNormals, computeSomeTangents);

    m_triangleCount = 0;
    for (int m = 0; m < m_meshArray.size(); ++m) {
        alwaysAssertM(m_meshArray[m]->primitive == PrimitiveType::TRIANGLES, 
            "Only implemented for PrimitiveType::TRIANGLES");
        m_triangleCount += m_meshArray[m]->cpuIndexArray.size() / 3;
    }
    
    if (computeSomeNormals || alwaysMergeVertices) {
        // Expand into an un-indexed triangle list.  This allows us to consider
        // each vertex's normal independently if needed.
        Array<Face> faceArray;
        Face::AdjacentFaceTable adjacentFaceTable;

        buildFaceArray(faceArray, adjacentFaceTable);

        if (computeSomeNormals) {
            // Angles smaller than this are considered to be curves and not creases
            const float maximumSmoothAngle = 45 * units::degrees();
            computeMissingVertexNormals(faceArray, adjacentFaceTable, maximumSmoothAngle);
        }

        // Merge vertices that have nearly equal normals, positions, and texcoords.
        // We no longer need adjacency information because tangents can be computed
        // solely from shared vertex information.
        alwaysAssertM(false, "TODO");
        mergeVertices(faceArray);
    }

    if (computeSomeTangents) {
        // Compute tangent space
        alwaysAssertM(false, "TODO");
    }
}


void ArticulatedModel2::Part::mergeVertices(const Array<Face>& faceArray) {
}

void ArticulatedModel2::Part::computeMissingVertexNormals
 (Array<Face>&                      faceArray, 
  const Face::AdjacentFaceTable&    adjacentFaceTable, 
  const float                       maximumSmoothAngle) {

    const float smoothThreshold = cos(maximumSmoothAngle);

    // Compute vertex normals as needed
    for (int f = 0; f < faceArray.size(); ++f) {
        Face& face = faceArray[f];

        for (int v = 0; v < 3; ++v) {
            Vertex& vertex = face.vertex[v];

            // Only process vertices with normals that have been flagged as NaN
            if (isNaN(vertex.normal.x)) {
                // This normal needs to be computed
                vertex.normal = Vector3::zero();
                const Face::IndexArray& faceIndexArray = adjacentFaceTable.get(vertex.position);
                for (int i = 0; i < faceIndexArray.size(); ++i) {
                    const Face& adjacentFace = faceArray[faceIndexArray[i]];
                    const float cosAngle = face.unitNormal.dot(adjacentFace.unitNormal);

                    // Only process if within the cutoff angle
                    if (cosAngle >= smoothThreshold) {
                        // These faces are close enough to be considered part of a
                        // smooth surface.  Add the non-unit normal
                        vertex.normal += adjacentFace.normal;
                    }
                }

                // Make the vertex normal unit length
                vertex.normal = vertex.normal.directionOrZero();
                debugAssertM(! vertex.normal.isNaN() && ! vertex.normal.isZero(),
                    "Smooth vertex normal produced an illegal value--"
                    "the adjacent face normals were probably corrupt");
            }
        }
    }
}


void ArticulatedModel2::Part::buildFaceArray(Array<Face>& faceArray, Face::AdjacentFaceTable& adjacentFaceTable) {
    adjacentFaceTable.clear();
    faceArray.fastClear();
    faceArray.reserve(m_triangleCount);

    // Maps positions to the faces adjacent to that position.  The valence of the average vertex in a closed mesh is 6, so
    // allocate slightly more indices so that we rarely need to allocate extra heap space.

    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];
        const Array<int>& indexArray = mesh->cpuIndexArray;

        // For every indexed triangle, create a Face
        for (int i = 0; i < indexArray.size(); i += 3) {
            const Face::Index faceIndex = faceArray.size();
            Face& face = faceArray.next();

            // Copy each vertex, updating the adjacency table
            for (int v = 0; v < 3; ++v) {
                int index = indexArray[i + v];
                face.vertex[v] = cpuVertexArray[index];

                TODO: we have to store the original index, so that we can perform a remapping of the tri lists later.

                // Record that this face is next to this vertex
                adjacentFaceTable.getCreate(face.vertex[v].position).append(faceIndex);
            }

            // Compute the non-unit and unit face normals
            face.normal = 
                (face.vertex[1].position - face.vertex[0].position).cross(
                    face.vertex[2].position - face.vertex[0].position);

            face.unitNormal = face.normal.directionOrZero();
        }
    }
}
