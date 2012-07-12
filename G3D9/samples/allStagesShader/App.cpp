/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    settings.window.width       = 960; 
    settings.window.height      = 600;
    settings.window.caption     = "All Stages Shader Demo";

#   ifdef G3D_WIN32
        // On Unix operating systems, icompile automatically copies data files.  
        // On Windows, we just run from the data directory.
        if (FileSystem::exists("data-files")) {
            chdir("data-files");
        } else if (FileSystem::exists("../samples/allStagesShader/data-files")) {
            chdir("../samples/allStagesShader/data-files");
        }

#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_innerTessLevel(1.0f), m_outerTessLevel(1.0f) {}

void App::makeGUI() {
     debugWindow->setVisible(true);
    
    
    GuiNumberBox<float>* innerSlider = debugPane->addNumberBox("Inner Tesselation Level", &m_innerTessLevel, "", GuiTheme::LINEAR_SLIDER, 1.0f, 20.f);
    innerSlider->setWidth(290.0f);
    innerSlider->setCaptionWidth(140.0f);
    GuiNumberBox<float>* outerSlider = debugPane->addNumberBox("Outer Tesselation Level", &m_outerTessLevel, "", GuiTheme::LINEAR_SLIDER, 1.0f, 20.f);
    outerSlider->setCaptionWidth(140.0f);
    outerSlider->setWidth(290.0f);
    debugPane->pack();
    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, window()->height()-debugWindow->rect().height(), 300, debugWindow->rect().height()));
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);
}

void App::onInit() {
    ArticulatedModel::Specification spec;
    spec.filename       = System::findDataFile("icosahedron/icosahedron.obj");

    ArticulatedModel::Ref model = ArticulatedModel::create(spec);
    model->pose(m_sceneGeometry);

    m_allStagesShader = Shader2::fromFiles("geodesic.vrt", "geodesic.ctl", "geodesic.evl", "geodesic.geo", "geodesic.pix");

    makeGUI();
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    rd->setColorClearValue(Color3::white() * 0.3f);
    rd->clear();

    
    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
    Args args;
    args.setUniform("TessLevelInner", m_innerTessLevel);
    args.setUniform("TessLevelOuter", m_outerTessLevel);
    args.geometryInput = PrimitiveType::PATCHES;
    args.patchVertices = 3;
    rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
    rd->setProjectionAndCameraMatrix(defaultCamera);

    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const SuperSurface::Ref&  surface = m_sceneGeometry[i].downcast<SuperSurface>();
        if(surface.isNull()) {
            debugPrintf("Surface %d, not a supersurface.\n", i);
            continue;
        }
        const SuperSurface::GPUGeom::Ref& gpuGeom = surface->gpuGeom();
        args.setStream("Position", gpuGeom->vertex);
        args.setIndexArray(gpuGeom->index);
        CoordinateFrame cf;
        surface->getCoordinateFrame(cf);
        rd->setObjectToWorldMatrix(cf);

        rd->apply(m_allStagesShader, args);
    }
    
}


bool App::onEvent(const GEvent& event) {
    if (GApp::onEvent(event)) {
        return true;
    }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'r')) { m_allStagesShader->reload(); return true; }
    return false;
}