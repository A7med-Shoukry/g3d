/**
 \file source/ArticulatedModel.cpp
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2003-09-14
 \edited  2011-06-18
 */

#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/IFSModel.h"
#include "G3D/ThreadSet.h"
#include "GLG3D/GLCaps.h"
#include "G3D/Any.h"
#include "G3D/Ray.h"
#include "G3D/FileSystem.h"
#include "GLG3D/BSPMAP.h"

namespace G3D {
        
ArticulatedModel::Specification::Specification() {}

ArticulatedModel::Specification::Specification(const Any& any) {
    *this = Specification();

    if (any.type() == Any::STRING) {
        filename = any.resolveStringAsFilename();
    } else {
        any.verifyName("ArticulatedModel::Specification");
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const std::string& key = toLower(it->key);
            if (key == "filename") {
                filename = it->value.resolveStringAsFilename();
            } else if (key == "preprocess") {
                preprocess = it->value;
            } else if (key == "settings") {
                settings = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
    }
}


Any ArticulatedModel::Specification::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::Specification");
    a.set("filename", filename);
    a.set("preprocess", preprocess);
    a.set("settings", settings);
    return a;
}

///////////////////////////////////////////////////////////

ArticulatedModel::Preprocess::Preprocess(const Any& any) {
    *this = Preprocess();
    any.verifyName("ArticulatedModel::Preprocess");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "stripmaterials") {
            stripMaterials = it->value.boolean();
        } else if (key == "texturedimension") {
            // TODO
        } else if (key == "addbumpmaps") {
            addBumpMaps = it->value.boolean();
        } else if (key == "replacetwosidedwithgeometry") {
            replaceTwoSidedWithGeometry = it->value.boolean();
        } else if (key == "xform") {
            xform = it->value;
        } else if (key == "parallaxsteps") {
            parallaxSteps = it->value;
        } else if (key == "bumpmapscale") {
            bumpMapScale = it->value;
        } else if (key == "normalmapwhiteheightinpixels") {
            normalMapWhiteHeightInPixels = it->value;
        } else if (key == "materialsubstitution") {
            for (Any::AnyTable::Iterator m = it->value.table().begin(); m.isValid(); ++m) {
                materialSubstitution.set(m->key, m->value);
            }
        } else if (key == "materialoverride") {
            setMaterialOverride(it->value);
        } else if (key == "program") {
            program.resize(it->value.size());
            for (int i = 0; i < program.size(); ++i) {
                program[i] = Operation::create(it->value[i]);
            }
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}


Any ArticulatedModel::Preprocess::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::Preprocess");
    a.set("stripMaterials", stripMaterials);

    if (m_hasMaterialOverride) {
        a.set("materialOverride", m_materialOverride);
    }
    // a["textureDimension"] = TODO
    a.set("addBumpMaps",    addBumpMaps);
    a.set("xform",          xform);
    a.set("parallaxSteps",  parallaxSteps);
    a.set("bumpMapScale",   bumpMapScale);
    a.set("normalMapWhiteHeightInPixels", normalMapWhiteHeightInPixels);
    a.set("replaceTwoSidedWithGeometry",  replaceTwoSidedWithGeometry);
    
    Any t(Any::TABLE);
    for (Table<std::string, Material::Specification>::Iterator it = materialSubstitution.begin(); it.isValid(); ++it) {
        t[it->key] = it->value;
    }
    a["materialSubstitution"] = t;

    return a;
}

///////////////////////////////////////////////////////////

ArticulatedModel::Settings::Settings(const Any& any) {
    *this = Settings();
    any.verifyName("ArticulatedModel::Settings");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "weld") {
            weld = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}

Any ArticulatedModel::Settings::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::Settings");
    a.set("weld", weld);
    return a;
}

//////////////////////////////////////////////////////////

ArticulatedModel::Ref ArticulatedModel::createHeightfield
(const Image1::Ref& height, float xExtent, float yExtent, float zExtent, const Vector2& texScale,
 bool spaceCentered, bool twoSided) {
    ArticulatedModel::Ref model = ArticulatedModel::createEmpty();
    ArticulatedModel::Part& part = model->partArray.next();
    ArticulatedModel::Part::TriList::Ref triList = part.newTriList();

    if (zExtent == finf()) {
        zExtent = xExtent;
    }

    MeshAlg::generateGrid(part.geometry.vertexArray, part.texCoordArray, triList->indexArray, 
                          height->width() - 1, height->height() - 1, texScale, 
                          spaceCentered, twoSided, CFrame(Matrix4::scale(xExtent, yExtent, zExtent).upper3x3()),
                          height);
    part.name = "Root";

    triList->primitive = PrimitiveType::TRIANGLES;
    triList->twoSided = false;

    model->updateAll();

    return model;
}

void ArticulatedModel::setStorage(ImageStorage s) {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        for (int t = 0; t < part.triList.size(); ++t) {
            part.triList[t]->material->setStorage(s);
        }
    }
}


bool ArticulatedModel::intersect
(const Ray& R, const CFrame& cframe, const Pose& pose, float& maxDistance, int& partIndex, 
 int& triListIndex, int& triIndex, float& u, float& v) const {
 
    // Take the ray to object space
    const Ray& osRay = cframe.toObjectSpace(R);

    const ArticulatedModel::Ref ptr = this;
    bool result = false;
    // Start with the roots
    for (int i = 0; i < partArray.size(); ++i) {
        if (partArray[i].parent == -1) {
            // This is a root
            if (partArray[i].intersect(osRay, i, ptr, pose, maxDistance, partIndex, triListIndex, triIndex, u, v)) {
                result = true;
            }
        }
    }
   
    return result;
}


int ArticulatedModel::partIndex(const PartID& id) const {
    if (id.m_index != USE_NAME) {
        return id.m_index;
    } else {
        for (int i = 0; i < partArray.size(); ++i) {
            if (partArray[i].name == id.m_name) {
                return i;
            }
        }
        debugAssertM(false, "No part named " + id.m_name);
    }
    return -1;
}

ArticulatedModel::Part::TriList::Ref ArticulatedModel::Part::newTriList(const Material::Ref& mat) {
    TriList::Ref t = new TriList();

    if (mat.isNull()) {
        Material::Specification s;
        s.setLambertian(Color3::white() * 0.8f);
        s.setSpecular(Color3::black());
        s.setShininess(0);

        t->material = Material::create(s);
    } else {
        t->material = mat;
    }

    triList.append(t);
    return t;
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Vector3& scale) {
    return fromFile(filename, Preprocess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const CoordinateFrame& xform) {
    return fromFile(filename, Preprocess(xform.toMatrix4()), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Matrix4& xform) {
    return fromFile(filename, Preprocess(xform), Settings());
}

ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, float scale) {
    return fromFile(filename, Preprocess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Preprocess& preprocess, const Settings& settings) {
    alwaysAssertM(FileSystem::exists(filename),
        filename + " cannot be loaded by ArticulatedModel because it does not exist.");

    ArticulatedModel::Ref model = new ArticulatedModel();
    model->setSettings(settings);

    std::string ext = FilePath::ext(toLower(filename));

    if (ext == "3ds") {
        model->init3DS(filename, preprocess);
    } else if (ext == "obj") {
        model->initOBJ(filename, preprocess);
    } else if ((ext == "ifs") || (ext == "ply2") || (ext == "ply")) {
        model->initIFS(filename, preprocess.xform);
    } else if (ext == "off") {
        model->initOFF(filename, preprocess);
    } else if (ext == "bsp") {
        model->initBSP(filename, preprocess);
    }

    if (preprocess.hasMaterialOverride()) {
        Material::Ref mo = Material::create(preprocess.materialOverride());
        for (int p = 0; p < model->partArray.size(); ++p) {
            Part& part = model->partArray[p];
            for (int t = 0; t < part.triList.size(); ++t) {
                part.triList[t]->material = mo;
            }
        }
    }

    // Run the program
    for (int i = 0; i < preprocess.program.size(); ++i) {
        preprocess.program[i]->apply(model);
    }

    if (preprocess.replaceTwoSidedWithGeometry) {
        model->replaceTwoSidedWithGeometry();
    }

    model->updateAll();

    return model;
}


void ArticulatedModel::replaceTwoSidedWithGeometry() {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        for (int t = 0; t < part.triList.size(); ++t) {
            Part::TriList::Ref triList = part.triList[t];
            if (triList.notNull() && triList->twoSided) {
                triList->twoSided = false;
                Array<int>& index = triList->indexArray;
                int N = index.size();
                debugAssert(triList->primitive = PrimitiveType::TRIANGLES);
                index.resize(2 * N);
                // Reverse
                for (int i = 0; i < N; ++i) {
                    index[i + N] = index[N - 1 - i];
                }
            }
        }
    }
}


ArticulatedModel::Ref ArticulatedModel::createEmpty() {
    return new ArticulatedModel();
}


void ArticulatedModel::initBSP(const std::string& filename, const Preprocess& preprocess) {
    Stopwatch s;
    std::string defaultTexture = "<white>";

    // TODO: make a load option
    Sphere keepOnly(Vector3::zero(), finf());

    // TODO: parse filename to find enclosing directory

    const std::string& pk3File = FilePath::parent(FilePath::parent(FileSystem::resolve(filename)));
    const std::string& bspFile = FilePath::baseExt(filename);

    // Load the Q3 format map    
    const BSPMapRef& src = BSPMap::fromFile(pk3File, bspFile, 1.0, "", defaultTexture);
    s.after("Load .bsp file");
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
    // s.after("Extract triangles");

    // Convert it to an ArticulatedModel (discarding light maps)
    ArticulatedModel::Settings settings;
    //    settings.weld.normalSmoothingAngle = 0; // Turn off smoothing
    setSettings(settings);

    name = bspFile;
    ArticulatedModel::Part& part = partArray.next();
    part.name = "root";

    // If skip.contains(textureFilename), then this surface should be removed from the map
    Set<std::string> skip;

    // TODO: add skip to preprocess
    
    // Maps texture names to triLists
    Table<std::string, ArticulatedModel::Part::TriList::Ref > triListTable;

    // There will be one part with many tri lists, one for each
    // texture.  Create those tri lists here.  Note that many textures
    // are simply "white".
    for (int i = 0; i < textureMapArray.size(); ++i) {
        if (! skip.contains(textureMapArray[i]->name())) {

            const Texture::Ref& lambertianTexture = textureMapArray[i];
            const std::string& name = lambertianTexture->name();

            // Only add textures not already present
            if (! triListTable.containsKey(name)) {

                const ArticulatedModel::Part::TriList::Ref& triList = part.newTriList();
                triListTable.set(name, triList);

                triList->twoSided = ! lambertianTexture->opaque();

                // Create the material for this part 
                
                // (TODO: material subsititution and override)
                const SuperBSDF::Ref& bsdf =
                    SuperBSDF::create(Component4(Color4::one(), lambertianTexture), 
                                     Color4::zero(), Color3::zero(), 1.0, Color3::black());
                triList->material = Material::create(bsdf);
            }
        }
    }
    // s.after("Create materials");

    part.geometry.vertexArray = vertexArray;
    part.geometry.normalArray = normalArray;
    part.texCoordArray        = texCoordArray;

    // Iterate over triangles, putting into the appropriate trilist based on their texture map index.
    const int numTris = textureMapIndexArray.size();
    for (int t = 0; t < numTris; ++t) {
        
        const int tlIndex = textureMapIndexArray[t];
        const std::string& name = textureMapArray[tlIndex]->name();

        if (! skip.contains(name)) {

            const ArticulatedModel::Part::TriList::Ref& triList = triListTable[name];

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
                    triList->indexArray.append(indexArray[i + j]);
                }
            }
        }
    }
    triListTable.clear();

    // Remove any triLists that were empty
    for (int t = 0; t < part.triList.size(); ++t) {
        if ((part.triList[t]->indexArray.size() == 0) ||
            (part.triList[t]->material->bsdf()->lambertian().max().a < 0.4f)) {
            part.triList.fastRemove(t);
            --t;
        /*} else {
            debugPrintf("Q3 parts kept: %s, %f\n", 
                part.triList[t]->material->bsdf()->lambertian().texture()->name().c_str(),
                part.triList[t]->material->bsdf()->lambertian().max().a);
                */
        }
    }
    // s.after("Create parts");
}


bool ArticulatedModel::Part::intersect
(const Ray& R, int myPartIndex, const ArticulatedModel::Ref& model, const Pose& pose, float& maxDistance,
 int& partIndex, int& triListIndex, int& triIndex, float& u, float& v) const {
    CoordinateFrame frame;

    if (pose.cframe.containsKey(name)) {
        frame = cframe * pose.cframe[name];
    } else {
        frame = cframe;
    }

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    const Ray& osRay = frame.toObjectSpace(R);

    bool result = false;
    if (hasGeometry()) {

        for (int t = 0; t < triList.size(); ++t) {
            const TriList::Ref& list = triList[t];

            const Point3* vertex = geometry.vertexArray.getCArray();

            if (list.notNull() && (list->indexArray.size() > 0) && (osRay.intersectionTime(list->boxBounds) < maxDistance)) {
                const Array<int>& indexArray = list->indexArray;
                const int N = indexArray.size();
                const int* index = indexArray.getCArray();

                // Check for intersections
                for (int i = 0; i < N; i += 3) {
                    
                    float w0 = 0, w1 = 0, w2 = 0;
                    const float temp = osRay.intersectionTime
                        (vertex[index[i]], 
                         vertex[index[i + 1]],
                         vertex[index[i + 2]],
                         w0,
                         w1,
                         w2);

                    if (temp < maxDistance) {
                        maxDistance = temp;
                        partIndex = myPartIndex;
                        triIndex = i;
                        triListIndex = t;
                        result = true;
                        u = w0;
                        v = w1;
                    }
                }
            }
        }
    }


    // Recursively check subparts and pass along our coordinate frame.
    for (int i = 0; i < subPartArray.size(); ++i) {
        const int p = subPartArray[i];
        debugAssertM(model->partArray[p].parent == myPartIndex,
            "Parent and child pointers do not match.");(void)myPartIndex;

        if (model->partArray[p].intersect(osRay, p, model, pose, maxDistance, partIndex, triListIndex, triIndex, u, v)) {
            result = true;
        }
    }

    return result;
}


void ArticulatedModel::Part::computeNormalsAndTangentSpace
    (const ArticulatedModel::Settings& settings) {

    if (triList.size() == 0) {
        geometry.vertexArray.resize(0);
        geometry.normalArray.resize(0);
        indexArray.resize(0);
        texCoordArray.resize(0);
        return;
    }

    Array<Array<int>*> indexArrayArray;
    indexArrayArray.resize(triList.size());
    for (int t = 0; t < triList.size(); ++t) {
        if (triList[t].notNull()) {
            indexArrayArray[t] = &(triList[t]->indexArray);
        } else {
            indexArrayArray[t] = NULL;
        }
    }

    if (geometry.vertexArray.size() > 0) {
        Welder::weld(geometry.vertexArray,
                     texCoordArray,
                     geometry.normalArray,
                     indexArrayArray,
                     settings.weld);
    }

    Array<MeshAlg::Face>    faceArray;
    Array<MeshAlg::Vertex>  vertexArray;
    Array<MeshAlg::Edge>    edgeArray;
    Array<Vector3>          faceNormalArray;

    Stopwatch s;
    computeIndexArray();

    MeshAlg::computeAdjacency(geometry.vertexArray, indexArray, faceArray, edgeArray, vertexArray);

    // Compute a tangent space basis
    if (texCoordArray.size() > 0) {
        // computeTangentSpaceBasis will generate binormals, but
        // we throw them away and recompute
        // them in the vertex shader.
        Array<Vector3> T;
        Array<Vector3> B;

        MeshAlg::computeTangentSpaceBasis(
            geometry.vertexArray,
            texCoordArray,
            geometry.normalArray,
            faceArray,
            T,
            B);

        // Pack the tangents 
        packedTangentArray.resize(T.size());
        for (int i = 0; i < T.size(); ++i) {
            const Vector3& t = T[i];
            const Vector3& b = B[i];
            const Vector3& n = geometry.normalArray[i];
            Vector4& p = packedTangentArray[i];
            p.x = t.x;
            p.y = t.y;
            p.z = t.z;
            p.w = sign(t.cross(b).dot(n));
        }
    } else {
        packedTangentArray.clear();
    }
}


void ArticulatedModel::Part::updateVAR(VertexBuffer::UsageHint hint) {
    if (geometry.vertexArray.size() == 0) {
        // Has no geometry
        return;
    }

    SuperSurface::CPUGeom g(NULL, &geometry, &texCoordArray, &packedTangentArray);
    g.copyVertexDataToGPU(vertexVAR, normalVAR, packedTangentVAR, texCoord0VAR, hint);

    for (int i = 0; i < triList.size(); ++i) {
        if (triList[i].notNull()) {
            triList[i]->updateVAR(hint, vertexVAR, normalVAR, packedTangentVAR, texCoord0VAR);
        }
    }
}


void ArticulatedModel::Part::computeBounds() {
    for (int t = 0; t < triList.size(); ++t) {
        if (triList[t].notNull()) {
            triList[t]->computeBounds(*this);
        }
    }
}

/** Used by ArticulatedModel::updateAll */
class PartUpdater : public GThread {
protected:

    Array<ArticulatedModel::Part*>&     m_partArray;
    int                                 m_startIndex;
    int                                 m_endIndex;
    const ArticulatedModel::Settings    m_settings;

public:
    /** Stores the pointer to partArray.*/
    PartUpdater(
        Array<ArticulatedModel::Part*>& partArray, 
        int startIndex, int endIndex,
        const ArticulatedModel::Settings& settings) :
        GThread("Part Updater"),
        m_partArray(partArray),
        m_startIndex(startIndex),
        m_endIndex(endIndex),
        m_settings(settings) {
    }

    /** Processes from startIndex to endIndex, inclusive. */
    virtual void threadMain() {
        for (int i = m_startIndex; i <= m_endIndex; ++i) {
            ArticulatedModel::Part* part = m_partArray[i];
            part->computeNormalsAndTangentSpace(m_settings);
            part->computeBounds();
            debugAssert(part->geometry.normalArray.size() ==
                        part->geometry.vertexArray.size());
        }
    }
};


void ArticulatedModel::updateAll() {
    // Extract the parts with real geometry
    Array<Part*> geometryPart;
    for (int p = 0; p < partArray.size(); ++p) {
        Part* part = &partArray[p];
        if (part->hasGeometry()) {
            geometryPart.append(part);
        } else {
            // Cheap to update this part right here, since it has nothing in it
            part->computeNormalsAndTangentSpace(m_settings);
            part->computeBounds();
            part->updateVAR();
        }
    }

    // Choose a reasonable number of threads
    int numThreads = 1;
    if ((geometryPart.size() >= 2) && (System::numCores() > 1)) {
        // Use at least two cores, and up to n-1 of them
        numThreads = min(geometryPart.size(), max(System::numCores() - 1, 2));
    }
    
    // Assign threads
    ThreadSet threads;
    int startIndex = 0;
    for (int t = 0; t < numThreads; ++t) {
        int endIndex =
            (numThreads == 1) ? 
            (geometryPart.size() - 1) :
            (geometryPart.size() - 1) * t / (numThreads - 1);
        GThread::Ref thread = new PartUpdater(geometryPart, startIndex, endIndex, m_settings);
        threads.insert(thread);
        startIndex = endIndex + 1;
    }
    debugAssertM(startIndex == geometryPart.size(), 
                 "Did not spawn threads for all parts");
    threads.start(USE_CURRENT_THREAD);
    threads.waitForCompletion();

    // Upload data to GPU
    for (int p = 0; p < geometryPart.size(); ++p) {
        Part* part = geometryPart[p];
        part->updateVAR();
    }

    m_numTriangles = 0;
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        m_numTriangles += part.indexArray.size() / 3;
    }

#   ifdef G3D_DEBUG
    // Check for correctness
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        debugAssert(part.geometry.normalArray.size() == part.geometry.vertexArray.size());
    }
#   endif
}


void ArticulatedModel::initIFS(const std::string& filename, const Matrix4& xform) {
    Array<int>      index;
    Array<Vector3>  vertex;
    Array<Vector2>  texCoord;
    
    IFSModel::load(filename, name, index, vertex, texCoord);

    // Transform vertices
    for (int v = 0; v < vertex.size(); ++v) {
        vertex[v] = xform.homoMul(vertex[v], 1.0f);
    }

    // Convert to a Part
    Part& part = partArray.next();

    part.cframe = CoordinateFrame();
    part.name = "root";
    part.parent = -1;
    part.geometry.vertexArray = vertex;
    part.texCoordArray = texCoord;

    Part::TriList::Ref triList = part.newTriList();
    triList->indexArray = index;
}


void ArticulatedModel::Part::TriList::updateVAR
    (VertexBuffer::UsageHint hint,
     const VertexRange& vertexVAR,
     const VertexRange& normalVAR,
     const VertexRange& tangentVAR,
     const VertexRange& texCoord0VAR) {

    vertex = vertexVAR;
    normal = normalVAR;
    packedTangent = tangentVAR;
    texCoord0 = texCoord0VAR;

    if (indexArray.size() == 0) {
        // Has no indices
        return;
    }

    int indexSize = sizeof(int);
    if (index.size() != indexArray.size()) {
        // Create new
        VertexBufferRef area = VertexBuffer::create(indexSize * indexArray.size(), hint, VertexBuffer::INDEX);
        index = VertexRange(indexArray, area);
    } else {
        // Update in place
        index.update(indexArray);
    }
}


void ArticulatedModel::Part::TriList::computeBounds(const Part& parentPart) {
    if (indexArray.size() > 0) { 
        MeshAlg::computeBounds(parentPart.geometry.vertexArray, indexArray, boxBounds, sphereBounds);
    } else {
        boxBounds = AABox();
        sphereBounds = Sphere();
    }
}


void ArticulatedModel::Part::computeIndexArray() {
    indexArray.clear();
    for (int t = 0; t < triList.size(); ++t) {
        if (triList[t].notNull()) {
            indexArray.append(triList[t]->indexArray);
        }
    }
}


static void addRect(const Vector3& v0, const Vector3& v1, 
                    const Vector3& v2, const Vector3& v3, 
                    Array<Vector3>& vertexArray, 
                    Array<int>& indexArray) {

    int v = vertexArray.size();
    vertexArray.append(v0, v1, v2, v3);

    indexArray.append(v + 0, v + 1, v + 2);
    indexArray.append(v + 0, v + 2, v + 3);
}


ArticulatedModel::Ref ArticulatedModel::createCornellBox(float scale, const Color3& left, const Color3& right, const Color3& walls) {

    ArticulatedModel::Ref model = ArticulatedModel::createEmpty();
    model->name = "Cornell Box";

    ArticulatedModel::Part& part = model->partArray.next();
    Array<Vector3>& vertex = part.geometry.vertexArray;
    part.name = "root";

    float c = -0.275f * scale;

    // Data used is captured from the photographs and balanced to
    // achieve (perceptual) uniform brightness on all surfaces; this
    // integrates the spectral data.

    // White faces
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(walls));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;

        // Top
        addRect(Vector3(-c,  c,  c), Vector3(-c,  c, -c), Vector3( c,  c, -c), Vector3( c,  c,  c), vertex, index);
    
        // Back
        addRect(Vector3(-c,  c, -c), Vector3(-c, -c, -c), Vector3( c, -c, -c), Vector3( c,  c, -c), vertex, index);

        // Floor
        addRect(Vector3( c, -c,  c), Vector3( c, -c, -c), Vector3(-c, -c, -c), Vector3(-c, -c,  c), vertex, index);
    }

    // Left red face
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(left));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;
        addRect(Vector3(-c,  c,  c), Vector3(-c, -c,  c), Vector3(-c, -c, -c), Vector3(-c,  c, -c), vertex, index);
    }

    // Right green face
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(right));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;
        addRect(Vector3( c,  c, -c), Vector3( c, -c, -c), Vector3( c, -c,  c), Vector3( c,  c,  c), vertex, index);
    }

    model->updateAll();

    return model;
}


void ArticulatedModel::facet() {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& dstPart = partArray[p];

        // Copy the old part
        Part srcPart = dstPart;

        dstPart.geometry.vertexArray.fastClear();
        dstPart.geometry.normalArray.fastClear();
        dstPart.texCoordArray.fastClear();
        dstPart.indexArray.fastClear();

        int n = 0;
        for (int t = 0; t < srcPart.triList.size(); ++t) {
            const Part::TriList::Ref srcTriList = srcPart.triList[t];
            Part::TriList::Ref dstTriList = dstPart.triList[t];
            dstTriList->indexArray.fastClear();

            // Unroll the arrays
            for (int x = 0; x < srcTriList->indexArray.size(); ++x) {
                int i = srcTriList->indexArray[x];

                // Create the vertex
                dstPart.geometry.vertexArray.append(srcPart.geometry.vertexArray[i]);
                if (srcPart.texCoordArray.size() > 0) {
                    dstPart.texCoordArray.append(srcPart.texCoordArray[i]);
                }
                dstPart.indexArray.append(n);
                dstTriList->indexArray.append(n);
                ++n;
            }
        }
    }
    updateAll();
}


const ArticulatedModel::Pose& ArticulatedModel::defaultPose() {
    static const Pose p;
    return p;
}


void ArticulatedModel::pose
(Array<Surface::Ref>&        posedArray, 
 const CoordinateFrame&      cframe, 
 const Pose&                 posex) {
    
    pose(posedArray, cframe, posex, cframe, posex);
}


void ArticulatedModel::pose
(Array<Surface::Ref>&        posedArray, 
 const CoordinateFrame&      cframe, 
 const Pose&                 posex,
 const CoordinateFrame&      previousCFrame,
 const Pose&                 previousPose) {
    
    for (int p = 0; p < partArray.size(); ++p) {
        const Part& part = partArray[p];
        if (part.parent == -1) {
            // This is a root part, pose it
            part.pose(this, p, posedArray, cframe, posex, previousCFrame, previousPose);
        }
    }
}


void ArticulatedModel::Part::pose
(const ArticulatedModel::Ref&      model,
 int                               partIndex,
 Array<Surface::Ref>&              posedArray,
 const CoordinateFrame&            parent,
 const Pose&                       posex,
 const CoordinateFrame&            previousParent,
 const Pose&                       previousPose) const {

    CoordinateFrame frame, previousFrame;

    if (posex.cframe.containsKey(name)) {
        frame = parent * cframe * posex.cframe[name];
    } else {
        frame = parent * cframe;
    }

    if (previousPose.cframe.containsKey(name)) {
        previousFrame = previousParent * cframe * previousPose.cframe[name];
    } else {
        previousFrame = previousParent * cframe;
    }

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    if (hasGeometry()) {

        for (int t = 0; t < triList.size(); ++t) {
            if (triList[t].notNull() && (triList[t]->indexArray.size() > 0)) {
                SuperSurface::CPUGeom cpuGeom(&triList[t]->indexArray, &geometry, 
                                              &texCoordArray, &packedTangentArray);

                posedArray.append(SuperSurface::create
                                  (model->name + format(".part[\"%s\"].triList[%d]", name.c_str(), t), 
                                   frame, previousFrame, triList[t], cpuGeom, model));
            }
        }
    }

    // Recursively pose subparts and pass along our coordinate frame.
    for (int i = 0; i < subPartArray.size(); ++i) {
        int p = subPartArray[i];
        debugAssertM(model->partArray[p].parent == partIndex,
            "Parent and child pointers do not match.");(void)partIndex;

        model->partArray[p].pose(model, p, posedArray, frame, posex, previousFrame, previousPose);
    }
}


void ArticulatedModel::initOFF(const std::string& filename, const Preprocess& preprocess) {
    
    TextInput::Settings s;
    s.cppBlockComments = false;
    s.cppLineComments = false;
    s.otherCommentCharacter = '#';

    TextInput ti(filename, s);

    // Based on http://www.geomview.org/docs/html/OFF.html
    
    ///////////////////////////////////////////////////////////////
    // Parse header
    std::string header = ti.readSymbol();
    bool hasTexCoords = false;
    bool hasColors = false;
    bool hasNormals = false;
    bool hasHomogeneous = false;
    bool hasHighDimension = false;

    if (beginsWith(header, "ST")) {
        hasTexCoords = true;
        header = header.substr(2);
    }
    if (beginsWith(header, "C")) {
        hasColors = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "N")) {
        hasNormals = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "4")) {
        hasHomogeneous = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "n")) {
        hasHighDimension = true;
        header = header.substr(1);
    }

    // Remaining header should be "OFF", but is not required according to the spec

    Token t = ti.peek();
    if ((t.type() == Token::SYMBOL) && (t.string() == "BINARY")) {
        throw std::string("BINARY OFF files are not supported by this version of G3D::ArticulatedModel");
    }

    int ndim = 3;
    if (hasHighDimension) {
        ndim = ti.readNumber();
    }
    if (hasHomogeneous) {
        ++ndim;
    }

    if (ndim < 3) {
        throw std::string("OFF files must contain at least 3 dimensions");
    }

    int nV = iFloor(ti.readNumber());
    int nF = iFloor(ti.readNumber());
    int nE = iFloor(ti.readNumber());
    (void)nE;

    ///////////////////////////////////////////////////

    // There is only one part, with one triList
    Part& part = partArray.next();
    Part::TriList::Ref triList = part.newTriList();

    Array<Vector3>& vertex = part.geometry.vertexArray;
    Array<Vector3>& normal = part.geometry.normalArray;
    Array<Vector2>& texCoord = part.texCoordArray;
    Array<int>& index = triList->indexArray;

    vertex.resize(nV);
    if (hasNormals) {
        normal.resize(nV);
    }

    if (hasTexCoords) {
        texCoord.resize(nV);
    }

    name = FilePath::baseExt(filename);

    Matrix3 normalXform;
    if (hasNormals) {
        normalXform = preprocess.xform.upper3x3().inverse().transpose();
    }

    // Read the per-vertex data
    for (int v = 0; v < nV; ++v) {

        // Vertex 
        for (int i = 0; i < 3; ++i) {
            vertex[v][i] = ti.readNumber();
        }

        vertex[v] = (preprocess.xform * Vector4(vertex[v], 1.0f)).xyz();

        // Ignore higher dimensions
        for (int i = 3; i < ndim; ++i) {
            ti.readNumber();
        }

        if (hasNormals) {
            // Normal (assume always 3 components)
            for (int i = 0; i < 3; ++i) {
                normal[v][i] = ti.readNumber();
            }

            normal[v] = (normalXform * normal[v]).direction();
        }

        if (hasColors) {
            // Color (assume always 3 components)
            for (int i = 0; i < 3; ++i) {
                ti.readNumber();
            }
        }

        if (hasTexCoords) {
            // Texcoords (assume always 2 components)
            for (int i = 0; i < 2; ++i) {
                texCoord[v][i] = ti.readNumber();
            }
        }
        // Skip to the end of the line.  If the file was corrupt we'll at least get the next vertex right
        ti.readUntilNewlineAsString();
    }

    // Faces

    // Convert arbitrary triangle fans to triangles
    Array<int> poly;
    for (int i = 0; i < nF; ++i) {
        poly.fastClear();
        int polySize = iFloor(ti.readNumber());
        debugAssert(polySize > 2);

        if (polySize == 3) {
            // Triangle (common case)
            for (int j = 0; j < 3; ++j) {
                index.append(iFloor(ti.readNumber()));
            }
        } else {
            poly.resize(polySize);
            for (int j = 0; j < polySize; ++j) {
                poly[j] = iFloor(ti.readNumber());
                debugAssertM(poly[j] < nV, 
                             "OFF file contained an index greater than the number of vertices."); 
            }

            // Expand the poly into triangles
            MeshAlg::toIndexedTriList(poly, PrimitiveType::TRIANGLE_FAN, index);
        }

        // Trim to the end of the line, except on the last line of the
        // file (where it doesn't matter)
        if (i != nF - 1) {
            // Ignore per-face colors
            ti.readUntilNewlineAsString();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////

ArticulatedModel::PoseSpline::PoseSpline() {}


ArticulatedModel::PoseSpline::PoseSpline(const Any& any) {
    any.verifyName("ArticulatedModel::PoseSpline");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        partSpline.getCreate(it->key) = it->value;
    }
}
 
void ArticulatedModel::PoseSpline::get(float t, ArticulatedModel::Pose& pose) {
    for (SplineTable::Iterator it = partSpline.begin(); it.isValid(); ++it) {
        if (it->value.control.size() > 0) {
            pose.cframe.set(it->key, it->value.evaluate(t));
        }
    }
}

} // G3D
