/**
 @file ArticulatedModel_3DS.cpp
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2003-09-14
 @edited  2010-09-27
 */

#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/IFSModel.h"
#include "Load3DS.h"
#include "G3D/ThreadSet.h"
#include "GLG3D/GLCaps.h"
#include "G3D/Any.h"
#include "G3D/FileSystem.h"
#include "GLG3D/BSPMAP.h"

namespace G3D {

/** If filename + any image extension exists, returns the name, otherwise returns "". */
static std::string findAnyImage(const std::string& filename) {
    static const char* ext[] = {"png", "jpg", "tga", "bmp", "pcx", ""};
    // TODO: Case sensitivity
    for (int i = 0; ext[i][0] != '\0'; ++i) {
        std::string tmp = filename + "." + ext[i];
        if (FileSystem::exists(tmp)) {
            return tmp;
        }
    }
    return "";
}

void ArticulatedModel::init3DS(const std::string& filename, const Preprocess& preprocess) {

    const Matrix4& xform = preprocess.xform;

    // Note: vertices are actually mutated by scale; it is not carried along as
    // part of the scene graph transformation.

    // Note: moving textures from CPU to GPU is the slow part of this process

    // Returns the index in partArray of the part with this name.
    Table<std::string, int>     partNameToIndex;

    // Cached bump maps; if NULL, there is no bump map for this file
    Table<std::string, Texture::Ref> bumpMap;

    //Table<std::string, TextureRef> texCache;

    std::string path = filenamePath(filename);
    Load3DS load;
    load.load(filename);

    partArray.resize(load.objectArray.size());

    // Rotation/scale component
    Matrix3 R = xform.upper3x3();

    Table<std::string, Material::Ref> materialSubstitution;
    for (Table<std::string, Material::Specification>::Iterator it = preprocess.materialSubstitution.begin(); it.hasMore(); ++it) {
        materialSubstitution.set(it->key, Material::create(it->value));
    }

    for (int p = 0; p < load.objectArray.size(); ++p) {
        Load3DS::Object& object = load.objectArray[p];

        Part& part = partArray[p];

        // Process geometry
        part.geometry.vertexArray = object.vertexArray;
        std::string name = object.name;
        int count = 0;
        while (partNameToIndex.containsKey(name)) {
            ++count;
            name = object.name + format("_#%d", count);
        }

        part.cframe = object.keyframe.approxCoordinateFrame();
        debugAssert(isFinite(part.cframe.rotation.determinant()));
        debugAssert(part.cframe.rotation.isOrthonormal());

        if (! part.cframe.rotation.isRightHanded()) {
            // TODO: how will this impact other code?  I think we can't just strong-arm it like this
            part.cframe.rotation.setColumn(0, -part.cframe.rotation.column(0));
        }

        debugAssert(part.cframe.rotation.isRightHanded());

        // Scale and rotate the cframe positions, but do not translate them
        part.cframe.translation = R * part.cframe.translation;

        debugAssert(R.column(0).isFinite());

        part.name = name;
        partNameToIndex.set(part.name, p);

        // All 3DS parts are promoted to the root in the current implementation.
        // TODO: this makes good animation impossible
        part.parent = -1;

        //debugPrintf("%s %d %d\n", object.name.c_str(), object.hierarchyIndex, object.nodeID);

        if (part.hasGeometry()) {
            // Convert vertices to object space (there is no surface normal data at this point)
            debugAssert(part.geometry.normalArray.size() == 0);
            Matrix4 netXForm = part.cframe.inverse().toMatrix4() * xform;
            
            debugAssertM(netXForm.row(3) == Vector4(0,0,0,1), 
                        "3DS file loading requires that the last row of the xform matrix be 0, 0, 0, 1");
            
            const Matrix3& S = netXForm.upper3x3();
            const Vector3& T = netXForm.column(3).xyz();
            for (int v = 0; v < part.geometry.vertexArray.size(); ++v) {
#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = part.geometry.vertexArray[v];
                    debugAssert(vec.isFinite());
                }
#               endif

                part.geometry.vertexArray[v] = S * part.geometry.vertexArray[v] + T;

#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = part.geometry.vertexArray[v];
                    debugAssert(vec.isFinite());
                }
#               endif
            }

            part.texCoordArray = object.texCoordArray;

            if (object.faceMatArray.size() == 0) {

                // Lump everything into one part
                Part::TriList::Ref triList = part.newTriList();
                triList->indexArray = object.indexArray;
                debugAssert(triList->indexArray.size() % 3 == 0);

            } else {
                for (int m = 0; m < object.faceMatArray.size(); ++m) {
                    const Load3DS::FaceMat& faceMat = object.faceMatArray[m];

                    if (faceMat.faceIndexArray.size() > 0) {

                        Material::Ref mat;
                        bool twoSided = false;

                        const std::string& materialName = faceMat.materialName;
                        if (load.materialNameToIndex.containsKey(materialName)) {
                            int i = load.materialNameToIndex[materialName];
                            const Load3DS::Material& material = load.materialArray[i];
                            
                            if (! materialSubstitution.get(material.texture1.filename, mat)) {
                                const Material::Specification& spec = compute3DSMaterial(&material, path, preprocess);
                                mat = Material::create(spec);
                            }
                            twoSided = material.twoSided || mat->hasAlphaMask();
                        } else {
                            mat = Material::create();
                            logPrintf("Referenced unknown material '%s'\n", materialName.c_str());
                        }                        

                        Part::TriList::Ref triList = part.newTriList(mat);
                        debugAssert(isValidHeapPointer(triList.pointer()));
                        triList->twoSided = twoSided;

                        // Construct an index array for this part
                        for (int i = 0; i < faceMat.faceIndexArray.size(); ++i) {
                            // 3*f is an index into object.indexArray
                            int f = faceMat.faceIndexArray[i];
                            debugAssert(f >= 0);
                            for (int v = 0; v < 3; ++v) {
                                triList->indexArray.append(object.indexArray[3 * f + v]);
                            }
                        }
                        debugAssert(triList->indexArray.size() > 0);
                        debugAssert(triList->indexArray.size() % 3 == 0);

                    } // if there are indices on this part
                } // for m
            } // if has materials 
        }
    }
}


static std::string find3DSTexture(std::string _filename, const std::string& path) {    
    if (_filename != "") {
        std::string filename = _filename;
        if (endsWith(toUpper(filename), "GIF")) {
            // Load PNG instead of GIF, since we can't load GIF
            filename = filename.substr(0, filename.length() - 3) + "png";
        }

        if (! FileSystem::exists(filename) && FileSystem::exists(pathConcat(path, filename))) {
            filename = pathConcat(path, filename);
        }

        // Load textures
        filename = System::findDataFile(filename, false);
        
        if (filename == "") {
            logPrintf("Could not locate 3DS file texture '%s'\n", _filename.c_str());
        }
        return filename;
    } else {
        return "";
    }
}

Material::Specification ArticulatedModel::compute3DSMaterial
(const void*         ptr,
 const std::string&  path,
 const Preprocess&   preprocess) {

    const Load3DS::Material& material = *reinterpret_cast<const Load3DS::Material*>(ptr);

    Material::Specification spec;

    if (preprocess.stripMaterials || preprocess.hasMaterialOverride()) {
        spec.setLambertian(Color3::one() * 0.7f);
        spec.setSpecular(Color3::one() * 0.2f);
        spec.setGlossyExponentShininess(100);
        return spec;
    }

    const Load3DS::Map& texture1 = material.texture1;

    const Color4& lambertianConstant = 
        Color4((material.diffuse * material.texture1.pct) *
               (1.0f - material.transparency), 1.0f);

    std::string lambertianFilename = find3DSTexture(texture1.filename, path);
    
    spec.setLambertian(lambertianFilename, lambertianConstant);

    // Strength of the shininess (higher is brighter)
    spec.setSpecular(max(material.shininessStrength * material.specular, Color3(material.reflection)) * (1.0f - material.transparency));

    if (material.reflection > 0.05f) {
        spec.setMirrorShininess();
    }

    //extent (area, higher is closely contained, lower is spread out) of shininess.
    // Don't scale up to the G3D maximum (1024) because 3DS files don't expect to ever be that shiny
    spec.setGlossyExponentShininess(material.shininess * 512);

    spec.setTransmissive(Color3::white() * material.transparency);
    spec.setEmissive(Color3::white() * material.emissive);

    std::string bumpFilename = find3DSTexture(material.bumpMap.filename, path);
    if (bumpFilename != "") {
        // TODO: use percentage specified in material.bumpMap
        spec.setBump(bumpFilename);
    }

    // TODO: load reflection, specular, etc maps.
    // triList->material.reflect.map = 

    if (preprocess.addBumpMaps) {
        // See if a bump map exists:
        std::string filename = 
            pathConcat(pathConcat(path, filenamePath(texture1.filename)),
                       filenameBase(texture1.filename) + "-bump");

        filename = findAnyImage(filename);
        if (filename != "") {
            BumpMap::Settings s;
            s.scale = preprocess.bumpMapScale;
            s.bias = 0;
            s.iterations = preprocess.parallaxSteps;
            spec.setBump(filename, s, preprocess.normalMapWhiteHeightInPixels);
        }
    } // if bump maps

    return spec;
}

}
