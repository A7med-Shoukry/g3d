/**
 \file GLG3D/source/ArticulatedModel2_OBJ.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"
#include "G3D/ParseOBJ.h"
#include "G3D/FileSystem.h"

namespace G3D {

static void stripMaterials(ParseOBJ& parseData) {
    ParseMTL::Material::Ref defaultMaterial = ParseMTL::Material::create();

    // Collapse the groups
    for (ParseOBJ::GroupTable::Iterator git = parseData.groupTable.begin();
        git.isValid();
        ++git) {
        ParseOBJ::Group::Ref& group = git->value;

        // For each mesh
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin();
            mit.isValid();
            ++mit) {

            ParseOBJ::Mesh::Ref& mesh = mit->value;
            mesh->material = defaultMaterial;
        }
    }
}


/** Rewrites the group table so that there is a single group per material, 
   and that group contains a single mesh. */
static void mergeGroupsAndMeshesByMaterial(ParseOBJ& parseData) {
    // Construct one mesh per material, and then rebuild the groups at the end
    ParseOBJ::MeshTable newMeshTable;

    // Collapse the groups
    for (ParseOBJ::GroupTable::Iterator git = parseData.groupTable.begin();
        git.isValid();
        ++git) {
        const ParseOBJ::Group::Ref& group = git->value;

        // For each mesh
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin();
            mit.isValid();
            ++mit) {

            const ParseOBJ::Mesh::Ref& srcMesh = mit->value;

            ParseOBJ::Mesh::Ref& dstMesh = newMeshTable.getCreate(srcMesh->material);
            dstMesh->material = srcMesh->material;
            dstMesh->faceArray.append(srcMesh->faceArray);
        }
    }

    // Rebuild the group table from the meshes
    parseData.groupTable.clear();
    for (ParseOBJ::MeshTable::Iterator mit = newMeshTable.begin();
        mit.isValid();
        ++mit) {

        ParseOBJ::Mesh::Ref& mesh = mit->value;
        ParseOBJ::Group::Ref group = ParseOBJ::Group::create();
        group->name = mesh->material->name;
        parseData.groupTable.set(group->name, group);
    }
}

/** Leaves empty filenames alone and resolves others */
static std::string resolveRelativeFilename(const std::string& filename, const std::string& basePath) {
    if (filename.empty()) {
        return filename;
    } else {
        return FileSystem::resolve(filename, basePath);
    }
}

/** \param basePath Resolve relative paths to here
*/
static Material::Specification toMaterialSpecification(const ParseMTL::Material::Ref& m) {
    Material::Specification s;

    s.setLambertian(resolveRelativeFilename(m->map_Kd, m->basePath), m->Kd);

    // TODO

    return s;
}


/** Flip texture coordiantes from the OBJ to the G3D convention */
inline static Point2 OBJToG3DTex(const Vector2& t) {
    return Vector2(t.x, 1.0f - t.y);
}

void ArticulatedModel2::loadOBJ(const Specification& specification) {
    // During loading, we make no attempt to optimize the mesh.  We leave that until the
    // Parts have been created.  The vertex arrays are therefore much larger than they
    // need to be.

    ParseOBJ parseData;
    {
        TextInput ti(specification.filename);
        parseData.parse(ti);
    }

    name = FilePath::base(specification.filename);

    Part* part = addPart(name);

    if (specification.stripMaterials) {
        stripMaterials(parseData);
    }

    if (specification.mergeMeshesByMaterial) {
        mergeGroupsAndMeshesByMaterial(parseData);
    }

    int numSpecifiedNormals = 0;
    int numSpecifiedTexCoord0s = 0;

    // All groups form a single AModel::Part.  Each mesh in each group
    // forms a single AModel::Mesh.
    for (ParseOBJ::GroupTable::Iterator git = parseData.groupTable.begin();
        git.isValid();
        ++git) {

        const ParseOBJ::Group::Ref& group = git->value;

        // For each mesh
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin();
            mit.isValid();
            ++mit) {

            const ParseOBJ::Mesh::Ref& srcMesh = mit->value;

            // Construct the AModel::Mesh for this group+mesh combination
            Mesh* mesh = addMesh(group->name + "/" + srcMesh->material->name, part);

            if (specification.stripMaterials) {
                // The default material
                mesh->material = Material::create();
            } else { 
                // The specified material.  G3D::Material will cache
                mesh->material = Material::create(toMaterialSpecification(srcMesh->material));
            }

            // For each face
            const Array<ParseOBJ::Face>& faceArray = srcMesh->faceArray;
            for (int f = 0; f < faceArray.size(); ++f) {
                const ParseOBJ::Face& face = faceArray[f];

                // Index of the first vertex that we'll add for this face
                const int prevNumVertices = part->cpuVertexArray.size();

                // For each vertex
                for (int v = 0; v < face.size(); ++v) {
                    const ParseOBJ::Index& index = face[v];
                    debugAssert(index.vertex != ParseOBJ::UNDEFINED);

                    CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex.next();

                    vertex.position = parseData.vertexArray[index.vertex];

                    if (index.normal != ParseOBJ::UNDEFINED) {
                        vertex.normal = parseData.normalArray[index.normal];
                        ++numSpecifiedNormals;
                    } else {
                        vertex.normal = Vector3::nan();
                    }

                    if (index.texCoord != ParseOBJ::UNDEFINED) {
                        vertex.texCoord0 = OBJToG3DTex(parseData.texCoordArray[index.texCoord]);
                    } else {
                        vertex.texCoord0 = Point2::zero();
                        ++numSpecifiedTexCoord0s;
                    }

                    // We have no tangent, so force it to NaN
                    vertex.tangent = Vector4::nan();
                } // for each vertex

                // Tessellate the polygon into triangles, writing to both the part index array
                // and the mesh index array.
                for (int t = 2; t < face.size(); ++t) {
                    int i = prevNumVertices + t - 2;
                    mesh->cpuIndexArray.append(i, i + 1, i + 2);
                } // for each triangle in the face
            } // for each face
        }
    }

    // If there are any texture coordinates, consider them all valid.  Only some of the
    // meshes may have texture coordinates, but those may need tangents and texcoords.
    part->m_hasTexCoord0 = (numSpecifiedTexCoord0s > 0);


    // Debugging code
    if (false) {
        // Code for dumping the imported vertices
        debugPrintf("** Vertices:\n");
        for (int i = 0; i < part->cpuVertexArray.vertex.size(); ++i) {
            const CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[i];
            debugPrintf(" %d: %s %s %s %s\n", i, 
                vertex.position.toString().c_str(), vertex.normal.toString().c_str(),
                vertex.tangent.toString().c_str(), vertex.texCoord0.toString().c_str());
        }
        debugPrintf("\n");

        // Code for dumping the imported indices
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
}

} // namespace G3D
