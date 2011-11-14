/**
 \file GLG3D/source/ArticulatedModel_OBJ.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-07-23
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel.h"
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

/** \param basePath Resolve relative paths to here
\cite http://www.fileformat.info/format/material/

MTL illum constants:
0	 Color on and Ambient off 

1	 Color on and Ambient on 

2	 Highlight on 

3	 Reflection on and Ray trace on 

4	 Transparency: Glass on 
     Reflection: Ray trace on 

5	 Reflection: Fresnel on and Ray trace on 

6	 Transparency: Refraction on 
     Reflection: Fresnel off and Ray trace on 

7	 Transparency: Refraction on 
     Reflection: Fresnel on and Ray trace on 

8	 Reflection on and Ray trace off 

9	 Transparency: Glass on 
     Reflection: Ray trace off 

10	 Casts shadows onto invisible surfaces 
*/
static Material::Specification toMaterialSpecification
(const ArticulatedModel::Specification& modelSpec, 
 const ParseMTL::Material::Ref&         m) {

    Material::Specification s;
    std::string filename;

    // Map OBJ model to G3D shading 
    filename = ArticulatedModel::resolveRelativeFilename(m->map_Kd, m->basePath);
    if (filename != "" && FileSystem::exists(filename)) {
        s.setLambertian(filename);
    } else {
        s.setLambertian(m->Kd);
    }

    filename = ArticulatedModel::resolveRelativeFilename(m->map_Ks, m->basePath);
    if (filename != "" && FileSystem::exists(filename)) {
        s.setSpecular(filename);
    } else {
        s.setSpecular(m->Ks.pow(9.0f) * 0.4f);
    }

    if (m->illum == 2 || m->illum == 10) {
        // [glossy] "hilight" on
        s.setGlossyExponentShininess(max(1.0f, m->Ns) * 100.0f);
    }

    if (m->illum == 4 || m->illum == 5 || m->illum == 6 || m->illum == 7) {
        // "ray trace" reflection on
        s.setMirrorShininess();
    }

    if (m->illum == 4 || m->illum == 6 || m->illum == 7 || m->illum == 9) {
        // Only apply transmission if the material
        s.setTransmissive(m->Tf);
        // Index of refraction (assume air)
        s.setEta(m->Ni, 1.0f);
    }

    // TODO: apply modelSpec options to bump map
    s.setBump(ArticulatedModel::resolveRelativeFilename(m->map_bump, m->basePath));

    s.setEmissive(m->Ke);

    return s;
}


/** Flip texture coordiantes from the OBJ to the G3D convention */
inline static Point2 OBJToG3DTex(const Vector2& t) {
    return Vector2(t.x, 1.0f - t.y);
}

void ArticulatedModel::loadOBJ(const Specification& specification) {
    // During loading, we make no attempt to optimize the mesh.  We leave that until the
    // Parts have been created.  The vertex arrays are therefore much larger than they
    // need to be.
    Stopwatch timer;

    ParseOBJ parseData;
    {
        BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);
        timer.after(" open file");
        parseData.parse(bi);
        timer.after(" parse");
        // Let the BinaryInput go out of scope, reclaiming its memory
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

        ParseOBJ::Group::Ref& group = git->value;

        // For each mesh
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin();
            mit.isValid();
            ++mit) {

            ParseOBJ::Mesh::Ref& srcMesh = mit->value;

            // Construct the AModel::Mesh for this group+mesh combination
            Mesh* mesh = addMesh(group->name + "/" + srcMesh->material->name, part);

            if (specification.stripMaterials) {
                // The default material
                mesh->material = Material::create();
            } else { 
                // The specified material.  G3D::Material will cache
                mesh->material = Material::create(toMaterialSpecification(specification, srcMesh->material));
            }

            // For each face
            Array<ParseOBJ::Face>& faceArray = srcMesh->faceArray;
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
                        ++numSpecifiedTexCoord0s;
                    } else {
                        vertex.texCoord0 = Point2::zero();
                    }

                    // We have no tangent, so force it to NaN
                    vertex.tangent = Vector4::nan();
                } // for each vertex

                // Tessellate the polygon into triangles, writing to both the part index array
                // and the mesh index array.
                for (int t = 2; t < face.size(); ++t) {
                    int i = prevNumVertices + t - 2;
                    mesh->cpuIndexArray.append(prevNumVertices, i + 1, i + 2);
                } // for each triangle in the face
            } // for each face

            // Remove old face data from memory to free space
            faceArray.clear(true);
        }
    }

    // If there are any texture coordinates, consider them all valid.  Only some of the
    // meshes may have texture coordinates, but those may need tangents and texcoords.
    part->m_hasTexCoord0 = (numSpecifiedTexCoord0s > 0);

    // Make any mesh that has partial coverage or transmission two-sided (OBJ-specific logic)
    for (int m = 0; m < part->m_meshArray.size(); ++m) {
        Mesh* mesh = part->m_meshArray[m];
        const Material::Ref material = mesh->material;
        const SuperBSDF::Ref bsdf = material->bsdf();

        if ((bsdf->lambertian().max().a < 1.0f) ||
            (bsdf->transmissive().max().max() > 0.0f)) {
            mesh->twoSided = true;
        }
    }

    // Debugging code
    if (false) {
        // Code for printing the imported vertices
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

    timer.after(" convert");
}

} // namespace G3D
