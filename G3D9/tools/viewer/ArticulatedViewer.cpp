/**
 \file ArticulatedViewer.cpp
 
 Viewer for .3ds models
 
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2011-06-12
 */
#include "ArticulatedViewer.h"

ArticulatedViewer::ArticulatedViewer() :
    m_model(NULL),
    m_numFaces(0),
    m_numVertices(0),
    m_selectedPart(NULL),
    m_selectedMesh(NULL),
    m_selectedTriangleIndex(0)
{}

static const float VIEW_SIZE = 10.0f;

void ArticulatedViewer::onInit(const std::string& filename) {

    m_selectedPart = NULL;
    m_selectedMesh = NULL;
    m_selectedTriangleIndex = -1;
    
    const RealTime start = System::time();
    if (toLower(filenameExt(filename)) == "any") {
        Any any;
        any.load(filename);

        m_model = ArticulatedModel2::create(ArticulatedModel2::Specification(any));
    } else {
        m_model = ArticulatedModel2::fromFile(filename);
    }
    debugPrintf("%s loaded in %f seconds\n", filename.c_str(), System::time() - start);

    Array<Surface::Ref> arrayModel;
    m_model->pose(arrayModel);

    m_model->countTrianglesAndVertices(m_numFaces, m_numVertices);
    
    m_scale = 1;
    m_offset = Vector3::zero();
    bool overwrite = true;
    
    // Find the size of the bounding box of the entire model
    AABox bounds;
    if (arrayModel.size() > 0) {
        
        for (int x = 0; x < arrayModel.size(); ++x) {		
            
            //merges the bounding boxes of all the seperate parts into the bounding box of the entire object
            AABox temp;
            CFrame cframe;
            arrayModel[x]->getCoordinateFrame(cframe);
            arrayModel[x]->getObjectSpaceBoundingBox(temp);
            Box partBounds = cframe.toWorldSpace(temp);
            
            // Some models have screwed up bounding boxes
            if (partBounds.extent().isFinite()) {
                if (overwrite) {
                    partBounds.getBounds(bounds);
                    overwrite = false;
                } else {
                    partBounds.getBounds(temp);
                    bounds.merge(temp);
                }
            }
        }
        
        if (overwrite) {
            // We never found a part with a finite bounding box
            bounds = AABox(Vector3::zero());
        }
        
        Vector3 extent = bounds.extent();
        Vector3 center = bounds.center();
        
        // Scale to X units
        float scale = 1.0f / max(extent.x, max(extent.y, extent.z));
        
        if (scale <= 0) {
            scale = 1;
        }

        if (! isFinite(scale)) {
            scale = 1;
        }

        m_scale = scale;
        m_offset = -scale * center;
        scale *= VIEW_SIZE;

        if (! center.isFinite()) {
            center = Vector3();
        }

        // Transform parts in-place
        ArticulatedModel2::ScaleTransformCallback scaleTransform(scale);
        m_model->forEachPart(scaleTransform);
        m_model->cleanGeometry();
    }

//    saveGeometry();
}


void ArticulatedViewer::saveGeometry() {
    /*
    const MeshAlg::Geometry& geometry = m_model->partArray[0].geometry;
    const Array<Point2>& texCoord     = m_model->partArray[0].texCoordArray;
    
    const ArticulatedModel::Part& part = m_model->partArray[0];

    int numIndices = 0;
    for (int t = 0; t < part.triList.size(); ++t) { 
        numIndices += part.triList[t]->indexArray.size();
    }

    BinaryOutput b("d:/out.bin", G3D_LITTLE_ENDIAN);
    b.writeInt32(numIndices);
    b.writeInt32(geometry.vertexArray.size());
    for (int t = 0; t < part.triList.size(); ++t) {
        const Array<int>& index = part.triList[t]->indexArray;
        for (int i = 0; i < index.size(); ++i) {
            b.writeInt32(index[i]);
        }
    }
    for (int i = 0; i < geometry.vertexArray.size(); ++i) {
        part.cframe.pointToWorldSpace(geometry.vertexArray[i]).serialize(b);
    }
    for (int i = 0; i < geometry.normalArray.size(); ++i) {
        part.cframe.vectorToWorldSpace(geometry.normalArray[i]).serialize(b);
    }
    if (texCoord.size() > 0) {
        for (int i = 0; i < texCoord.size(); ++i) {
            texCoord[i].serialize(b);
        }
    } else {
        for (int i = 0; i < geometry.vertexArray.size(); ++i) {
            Point2::zero().serialize(b);
        }
    }
    b.commit();
    */
}


static void printHierarchy
(const ArticulatedModel2::Ref& model,
 ArticulatedModel2::Part*      part,
 const std::string&            indent) {
    
    screenPrintf("%s\"%s\" (ID %d)\n", indent.c_str(), part->name.c_str(), (int)part->id);
    for (int i = 0; i < part->meshArray().size(); ++i) {
        screenPrintf("%s  Mesh \"%s\" (ID %d)\n", indent.c_str(), part->meshArray()[i]->name.c_str(),
                     (int)part->meshArray()[i]->id);
    }

    for (int i = 0; i < part->childArray().size(); ++i) {
        printHierarchy(model, part->childArray()[i], indent + "  ");
    }
}


void ArticulatedViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    // Separate and sort the models
    static Array<G3D::Surface::Ref> posed3D;
    
    m_model->pose(posed3D, m_offset);
    Surface::sortAndRender(rd, app->defaultCamera, posed3D, lighting, app->shadowMap);
    //Surface::renderWireframe(rd, posed3D);

    if (m_selectedMesh != NULL) {
        // Find the index array that matches the selected mesh and render it
        for (int p = 0; p < posed3D.size(); ++p) {
            SuperSurface::Ref s = posed3D[p].downcast<SuperSurface>();
            if (s->gpuGeom()->index == m_selectedMesh->gpuIndexArray) {
                // These have the same index array, so they must be the same surface
                rd->pushState(); {
                    CFrame cframe;
                    s->getCoordinateFrame(cframe);
                    rd->setObjectToWorldMatrix(cframe);
                    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
                    rd->setPolygonOffset(-1.0f);
                    rd->setColor(Color3::green() * 0.8f);
                    rd->setTexture(0, NULL);
                    s->sendGeometry(rd);
                } rd->popState();
                break;
            }
        }
    }
    posed3D.fastClear();

    screenPrintf("[Shown scaled by %f and offset by (%f, %f, %f)]\n",
                 m_scale, m_offset.x, m_offset.y, m_offset.z);
    
    screenPrintf("Model Faces: %d,  Vertices: %d\n", m_numFaces, m_numVertices);
    if (m_selectedPart != NULL) {
        screenPrintf(" Selected Part `%s' (ID %d), Mesh `%s' (ID %d), cpuIndexArray[%d...%d]\n", 
                     m_selectedPart->name.c_str(), 
                     (int)m_selectedPart->id,
                     m_selectedMesh->name.c_str(), 
                     (int)m_selectedMesh->id,
                     m_selectedTriangleIndex, m_selectedTriangleIndex + 2);
        screenPrintf(" Selected part->cframe = %s\n",
                     m_selectedPart->cframe.toXYZYPRDegreesString().c_str());
    }

    screenPrintf("Hierarchy:");
    // Hierarchy (could do this with a PartCallback)
    for (int i = 0; i < m_model->rootArray().size(); ++i) {
        printHierarchy(m_model, m_model->rootArray()[i], "");
    }
}


bool ArticulatedViewer::onEvent(const GEvent& e, App* app) {
    if ((e.type == GEventType::MOUSE_BUTTON_CLICK) && (e.button.button == 0)) {
        // Intersect all tri lists with the ray from the camera
        const Ray& ray = app->defaultCamera.worldRay(e.button.x, e.button.y, 
            app->renderDevice->viewport());

        m_selectedPart = NULL;
        m_selectedMesh = NULL;
        m_selectedTriangleIndex = -1;
        float u = 0, v = 0;

        float distance = finf();
        const bool hit = m_model->intersect(ray, m_offset, ArticulatedModel2::defaultPose(), distance, 
                                            m_selectedPart, m_selectedMesh, m_selectedTriangleIndex,
                                            u, v);
        return hit;
    }

    return false;
}
