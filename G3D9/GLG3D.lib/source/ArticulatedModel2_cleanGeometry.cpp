/**
 \file GLG3D/source/ArticulatedModel2_cleanGeometry.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-18
 \edited  2011-07-23
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/AreaMemoryManager.h"

namespace G3D {

void ArticulatedModel2::cleanGeometry(const CleanGeometrySettings& settings) {
    for (int p = 0; p < m_partArray.size(); ++p) {
        m_partArray[p]->cleanGeometry(settings);
    }
    computePartBounds();
}


void ArticulatedModel2::Part::cleanGeometry(const CleanGeometrySettings& settings) {
    Stopwatch timer;
    clearVertexRanges();

    bool computeSomeNormals = false, computeSomeTangents = false;
    determineCleaningNeeds(computeSomeNormals, computeSomeTangents);
    timer.after("  determineCleaningNeeds");

    m_triangleCount = 0;
    for (int m = 0; m < m_meshArray.size(); ++m) {
        alwaysAssertM(m_meshArray[m]->primitive == PrimitiveType::TRIANGLES, 
            "Only implemented for PrimitiveType::TRIANGLES");
        m_triangleCount += m_meshArray[m]->cpuIndexArray.size() / 3;
    }
    timer.after("  m_triangleCount");
    
    if (computeSomeNormals || settings.forceVertexMerging) {
        // Expand into an un-indexed triangle list.  This allows us to consider
        // each vertex's normal independently if needed.
        Array<Face> faceArray;
        Face::AdjacentFaceTable adjacentFaceTable;
        adjacentFaceTable.clearAndSetMemoryManager(AreaMemoryManager::create());

        buildFaceArray(faceArray, adjacentFaceTable);
        timer.after("  buildFaceArray");

        if (computeSomeNormals) {
            computeMissingVertexNormals(faceArray, adjacentFaceTable, settings.maxSmoothAngle);
            timer.after("  computeMissingVertexNormals");
        }
    
        // Merge vertices that have nearly equal normals, positions, and texcoords.
        // We no longer need adjacency information because tangents can be computed
        // solely from shared vertex information.
        mergeVertices(faceArray, settings.maxNormalWeldAngle);
        timer.after("  mergeVertices");
    }
    timer.after("  deallocation of adjacentFaceTable");

    if (computeSomeTangents) {
        // Compute tangent space
        computeMissingTangents();
        timer.after("  computeMissingTangents");
    }

    cpuVertexArray.hasTexCoord0 = m_hasTexCoord0;
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
        if (isNaN(cpuVertexArray.vertex[i].normal.x)) {
            computeSomeNormals = true;
            computeSomeTangents = true;
            // Wipe out the corresponding tangent vector
            cpuVertexArray.vertex[i].tangent.x = fnan();
        }
    }
    
    // See if tangents are needed
    if (! computeSomeTangents) {
        // Maybe there is a NaN tangent in there
        for (int i = 0; i < cpuVertexArray.size(); ++i) {
            if (isNaN(cpuVertexArray.vertex[i].tangent.x)) {
                computeSomeTangents = true;
                break;
            }
        }
    }    
}


void ArticulatedModel2::Part::debugPrint() const {
    const Part* part = this;
    // Code for dumping the vertices
    debugPrintf("** Vertices:\n");
    for (int i = 0; i < part->cpuVertexArray.vertex.size(); ++i) {
        const CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[i];
        debugPrintf(" %d: %s %s %s %s\n", i, 
            vertex.position.toString().c_str(), vertex.normal.toString().c_str(),
            vertex.tangent.toString().c_str(), vertex.texCoord0.toString().c_str());
    }
    debugPrintf("\n");

    // Code for dumping the indices
    debugPrintf("** Indices:\n");
    for (int m = 0; m < part->m_meshArray.size(); ++m) {
        const Mesh* mesh = part->m_meshArray[m];
        debugPrintf(" Mesh %s\n", mesh->name.c_str());
        for (int i = 0; i < mesh->cpuIndexArray.size(); i += 3) {
            debugPrintf(" %d-%d: %d %d %d\n", i, i + 2, mesh->cpuIndexArray[i], mesh->cpuIndexArray[i + 1], mesh->cpuIndexArray[i + 2]);
        }
        debugPrintf("\n");
    }
    debugPrintf("\n");
}


void ArticulatedModel2::computePartBounds() {
    for (int p = 0; p < m_partArray.size(); ++p) {
        Part* part = m_partArray[p];
        const CPUVertexArray::Vertex* vertexArray = part->cpuVertexArray.vertex.getCArray();

        part->boxBounds = AABox::empty();

        for (int m = 0; m < part->m_meshArray.size(); ++m) {
            Mesh* mesh = part->m_meshArray[m];
            const Array<int>& indexArray = mesh->cpuIndexArray;
            const int* index = indexArray.getCArray();

            AABox meshBounds;
            for (int i = 0; i < indexArray.size(); ++i) {
                meshBounds.merge(vertexArray[index[i]].position);
            }

            mesh->boxBounds = meshBounds;
            meshBounds.getBounds(mesh->sphereBounds);
            part->boxBounds.merge(meshBounds);
        }

        part->boxBounds.getBounds(part->sphereBounds);
    }
}


void ArticulatedModel2::Part::computeMissingTangents() {

    if (! m_hasTexCoord0) {
        cpuVertexArray.hasTangent = false;
        // If we have no texture coordinates, we are unable to compute tangents.   
        for (int v = 0; v < cpuVertexArray.size(); ++v) {
            cpuVertexArray.vertex[v].tangent = Vector4::zero();
        }
        return;
    }

    cpuVertexArray.hasTangent = true;
    alwaysAssertM(m_hasTexCoord0, "Cannot compute tangents without some texture coordinates.");
 
    // Compute all tangents, but only extract those that we need at the bottom.

    // See http://www.terathon.com/code/tangent.html for a derivation of the following code
    Array<Vector3>  tangent1;
    Array<Vector3>  tangent2;
    tangent1.resize(cpuVertexArray.size());
    tangent2.resize(cpuVertexArray.size());
    Vector3* tan1 = tangent1.getCArray();
    Vector3* tan2 = tangent2.getCArray();
    CPUVertexArray::Vertex* vertexArray = cpuVertexArray.vertex.getCArray();
    debugAssertM(tan1[0].x == 0, "This implementation assumes that new Vector3 values are initialized to zero.");

    // For each face
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];        
        const Array<int>& cpuIndexArray = mesh->cpuIndexArray;
        const int* indexArray = cpuIndexArray.getCArray();

        for (int i = 0; i < cpuIndexArray.size(); i += 3) {
            const int i0 = indexArray[i];
            const int i1 = indexArray[i + 1];
            const int i2 = indexArray[i + 2];
        
            const CPUVertexArray::Vertex& vertex0 = vertexArray[i0];
            const CPUVertexArray::Vertex& vertex1 = vertexArray[i1];
            const CPUVertexArray::Vertex& vertex2 = vertexArray[i2];

            const Point3& v0 = vertex0.position;
            const Point3& v1 = vertex1.position;
            const Point3& v2 = vertex2.position;
        
            const Point2& w0 = vertex0.texCoord0;
            const Point2& w1 = vertex1.texCoord0;
            const Point2& w2 = vertex2.texCoord0;
        
            const float x0 = v1.x - v0.x;
            const float x1 = v2.x - v0.x;
            const float y0 = v1.y - v0.y;
            const float y1 = v2.y - v0.y;
            const float z0 = v1.z - v0.z;
            const float z1 = v2.z - v0.z;
        
            const float s0 = w1.x - w0.x;
            const float s1 = w2.x - w0.x;
            const float t0 = w1.y - w0.y;
            const float t1 = w2.y - w0.y;
        
            const float r = 1.0f / (s0 * t1 - s1 * t0);
            
            const Vector3 sdir
                ((t1 * x0 - t0 * x1) * r, 
                 (t1 * y0 - t0 * y1) * r,
                 (t1 * z0 - t0 * z1) * r);

            const Vector3 tdir
                ((s0 * x1 - s1 * x0) * r, 
                 (s0 * y1 - s1 * y0) * r,
                 (s0 * z1 - s1 * z0) * r);
        
            tan1[i0] += sdir;
            tan1[i1] += sdir;
            tan1[i2] += sdir;
        
            tan2[i0] += tdir;
            tan2[i1] += tdir;
            tan2[i2] += tdir;        
        } // For each triangle
    } // For each mesh

    
    for (int v = 0; v < cpuVertexArray.size(); ++v) {
        CPUVertexArray::Vertex& vertex = vertexArray[v];

        if (isNaN(vertex.tangent.x)) {
            // This tangent needs to be overriden
            const Vector3& n = vertex.normal;
            const Vector3& t1 = tan1[v];
            const Vector3& t2 = tan2[v];
        
            // Gram-Schmidt orthogonalize
            const Vector3& T = (t1 - n * n.dot(t1)).directionOrZero();

            vertex.tangent.x = T.x;
            vertex.tangent.y = T.y;
            vertex.tangent.z = T.z;

            // Calculate handedness
            vertex.tangent.w = (n.cross(t1).dot(t2) < 0.0f) ? 1.0f : -1.0f;
        } // if this must be updated
    } // for each vertex
 }



/** Tracks if position and texcoord0 match, but ignores normals and tangents */
struct AM2VertexHash { 
    static size_t hashCode(const CPUVertexArray::Vertex& vertex) {
        return vertex.position.hashCode() ^ vertex.texCoord0.hashCode();
    }
    static bool equals(const CPUVertexArray::Vertex& a, const CPUVertexArray::Vertex& b) {
        return (a.position == b.position) && (a.texCoord0 == b.texCoord0);
    }
};


void ArticulatedModel2::Part::mergeVertices(const Array<Face>& faceArray, float maxNormalWeldAngle) {
    // Clear all mesh index arrays
    for (int m = 0; m < m_meshArray.size(); ++m) {
        Mesh* mesh = m_meshArray[m];
        mesh->cpuIndexArray.fastClear();
        mesh->gpuIndexArray = VertexRange();
    }

    // Clear the CPU vertex array
    cpuVertexArray.vertex.fastClear();

    Stopwatch timer;

    // Track the location of vertices in cpuVertexArray by their exact texcoord and position.
    // The vertices in the list may have differing normals.
    typedef int VertexIndex;
    typedef SmallArray<VertexIndex, 4> VertexIndexList;
    Table<CPUVertexArray::Vertex, VertexIndexList, AM2VertexHash, AM2VertexHash> vertexIndexTable;

    // Almost all of the time in this method is spent deallocating the table at
    // the end, so use an AreaMemoryManager to directly dump the allocated memory
    // without freeing individual objects.
    vertexIndexTable.clearAndSetMemoryManager(AreaMemoryManager::create());

    const float normalClosenessThreshold = cos(maxNormalWeldAngle);

    // Iterate over all faces
    int longestListLength = 0;
    for (int f = 0; f < faceArray.size(); ++f) {
        const Face& face = faceArray[f];
        Mesh* mesh = face.mesh;
        for (int v = 0; v < 3; ++v) {
            const CPUVertexArray::Vertex& vertex = face.vertex[v];

            // Find the location of this vertex in cpuVertexArray...or add it.
            // The texture coordinates and vertices must exactly match.
            // The normals may be slightly off, since the order of computation can affect them
            // even if we wanted no normal welding.
            VertexIndexList& list = vertexIndexTable.getCreate(vertex);

            int index = -1;
            for (int i = 0; i < list.size(); ++i) {
                int j = list[i];
                // See if the normals are close (we know that the texcoords and positions match exactly)
                if (cpuVertexArray.vertex[j].normal.dot(vertex.normal) >= normalClosenessThreshold) { 
                    // Reuse this vertex
                    index = j;
                    break;
                }
            }

            if (index == -1) {
                // This must be a new vertex, so add it
                index = cpuVertexArray.size();
                cpuVertexArray.vertex.append(vertex);
                list.append(index);
                longestListLength = max(longestListLength, list.size());
            }

            // Add this vertex index to the mesh
            mesh->cpuIndexArray.append(index);
        }
    }

   // debugPrintf("average bucket size = %f\n", vertexIndexTable.debugGetAverageBucketSize());
   // debugPrintf("deepest bucket size = %d\n", vertexIndexTable.debugGetDeepestBucketSize());
   // debugPrintf("longestListLength = %d\n", longestListLength);
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
            CPUVertexArray::Vertex& vertex = face.vertex[v];

            // Only process vertices with normals that have been flagged as NaN
            if (isNaN(vertex.normal.x)) {
                // This normal needs to be computed
                vertex.normal = Vector3::zero();
                const Face::IndexArray& faceIndexArray = adjacentFaceTable.get(vertex.position);

                if (face.unitNormal.isZero()) {
                    // This face has no normal (presumably it is degenerate), so just average adjacent ones directly
                    for (int i = 0; i < faceIndexArray.size(); ++i) {
                        vertex.normal += faceArray[faceIndexArray[i]].normal;
                    }
                } else {
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
                }

                // Make the vertex normal unit length
                vertex.normal = vertex.normal.direction();
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
        Mesh* mesh = m_meshArray[m];
        const Array<int>& indexArray = mesh->cpuIndexArray;

        // For every indexed triangle, create a Face
        for (int i = 0; i < indexArray.size(); i += 3) {
            const Face::Index faceIndex = faceArray.size();
            Face& face = faceArray.next();
            face.mesh = mesh;

            // Copy each vertex, updating the adjacency table
            for (int v = 0; v < 3; ++v) {
                int index = indexArray[i + v];
                face.vertex[v] = cpuVertexArray.vertex[index];

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

} // namespace G3D
