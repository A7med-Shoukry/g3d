/**
 \file GLG3D/source/ArticulatedModel_BSP.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-08-23
 \edited  2011-08-23
 
 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel.h"
#include "G3D/FileSystem.h"
#include "GLG3D/BSPMap.h"

namespace G3D {

void ArticulatedModel::loadBSP(const Specification& specification) {
    std::string defaultTexture = "<white>";

    // TODO: make a load option
    Sphere keepOnly(Vector3::zero(), finf());

    // Parse filename to find enclosing directory
    const std::string& pk3File = FilePath::parent(FilePath::parent(FileSystem::resolve(specification.filename)));
    const std::string& bspFile = FilePath::baseExt(specification.filename);

    // Load the Q3-format map    
    const BSPMapRef& src = BSPMap::fromFile(pk3File, bspFile, 1.0, "", defaultTexture);
    debugAssertM(src.notNull(), "Could not find " + pk3File);

    Array< Vector3 >    vertexArray;
    Array< Vector3 >   	normalArray;
    Array< int >  	indexArray;
    Array< Vector2 >    texCoordArray;
    Array< int >        textureMapIndexArray;
    Array< Vector2 >    lightCoordArray;
    Array< int >        lightMapIndexArray;
    Array< Texture::Ref > textureMapArray;
    Array< Texture::Ref > lightMapArray;
    
    src->getTriangles(vertexArray, normalArray, indexArray, texCoordArray,
                      textureMapIndexArray, lightCoordArray, lightMapIndexArray,
                      textureMapArray, lightMapArray);

    // Convert it to an ArticulatedModel (discarding light maps)
    name = bspFile;
    Part* part = addPart("root");
    // If skip.contains(textureFilename), then this surface should be removed from the map
    Set<std::string> skip;

    // TODO: add skip to preprocess
    
    // Maps texture names to triLists
    Table<std::string, Mesh*> triListTable;

    // There will be one part with many tri lists, one for each
    // texture.  Create those tri lists here.  Note that many textures
    // are simply "white".
    for (int i = 0; i < textureMapArray.size(); ++i) {
        if (! skip.contains(textureMapArray[i]->name())) {

            const Texture::Ref& lambertianTexture = textureMapArray[i];
            const std::string& name = lambertianTexture->name();

            // Only add textures not already present
            if (! triListTable.containsKey(name)) {
                Mesh* mesh = addMesh(name, part);
                triListTable.set(name, mesh);

                mesh->twoSided = ! lambertianTexture->opaque();

                // Create the material for this part 
                const SuperBSDF::Ref& bsdf =
                    SuperBSDF::create(Component4(Color4::one(), lambertianTexture), 
                                     Color4::zero(), Color3::zero(), 1.0, Color3::black());
                mesh->material = Material::create(bsdf);
            }
        }
    }

    Array<CPUVertexArray::Vertex>& vertex = part->cpuVertexArray.vertex;
    vertex.resize(vertexArray.size());
    for (int v = 0; v < vertex.size(); ++v) {
        CPUVertexArray::Vertex& vtx = vertex[v];
        vtx.position  = vertexArray[v];
        vtx.normal    = normalArray[v];
        vtx.texCoord0 = texCoordArray[v];
        vtx.tangent   = Vector4::nan();
    }
    part->cpuVertexArray.hasTangent   = false;
    part->cpuVertexArray.hasTexCoord0 = true;
    part->m_hasTexCoord0 = true;

    // Iterate over triangles, putting into the appropriate trilist based on their texture map index.
    const int numTris = textureMapIndexArray.size();
    for (int t = 0; t < numTris; ++t) {
        
        const int tlIndex = textureMapIndexArray[t];
        const std::string& name = textureMapArray[tlIndex]->name();

        if (! skip.contains(name)) {

            Mesh* mesh = triListTable[name];

            const int i = t * 3;

            bool keep = true;

            if (keepOnly.radius < inf()) {
                // Keep only faces that have at least one vertex within the sphere
                keep = false;
                for (int j = 0; j < 3; ++j) {
                    if (keepOnly.contains(vertexArray[indexArray[i + j]])) {
                        keep = true;
                        break;
                    }
                }
            }

            if (keep) {
                // Copy the indices of the triangle's vertices
                int i = t * 3;
                for (int j = 0; j < 3; ++j) {
                    mesh->cpuIndexArray.append(indexArray[i + j]);
                }
            }
        }
    }
    triListTable.clear();

    // Remove any meshes that were empty
    for (int t = 0; t < part->m_meshArray.size(); ++t) {
        if ((part->m_meshArray[t]->cpuIndexArray.size() == 0) ||
            (part->m_meshArray[t]->material->bsdf()->lambertian().max().a < 0.4f)) {
            part->m_meshArray.fastRemove(t);
            --t;
        /*} else {
            debugPrintf("Q3 parts kept: %s, %f\n", 
                part.triList[t]->material->bsdf()->lambertian().texture()->name().c_str(),
                part.triList[t]->material->bsdf()->lambertian().max().a);
                */
        }
    }
}

} // namespace G3D
