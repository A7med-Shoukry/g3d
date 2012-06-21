#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char** argv) {
    GApp::Settings settings(argc, argv);

    settings.window.caption = "G3D MultiView Demo";
    settings.window.width       = 1280; 
    settings.window.height      = 720;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
}


void App::onInit() {
    renderDevice->setColorClearValue(Color3::white());
    debugWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->cameraControlWindow->moveTo(Vector2(developerWindow->cameraControlWindow->rect().x0(), 0));
    developerWindow->setVisible(false);
    showRenderingStats = false;
    
    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(-0.61369f, 0.734589f, 0.934322f, 314.163f, -12.1352f));

    m_sao = SAO::create();
    m_shadowMap = ShadowMap::create();
    m_aoTexture = Texture::createEmpty("AO", window()->width(), window()->height(), 
        GLCaps::supportsTextureDrawBuffer(ImageFormat::R8()) ? ImageFormat::R8() : ImageFormat::RGB8(), Texture::DIM_2D_NPOT,
        Texture::Settings::buffer());
    m_aoFramebuffer = Framebuffer::create("AO");
    m_aoFramebuffer->set(Framebuffer::COLOR0, m_aoTexture);
    m_aoFramebuffer->set(Framebuffer::DEPTH,  m_depthBuffer);
    renderDevice->push2D(m_aoFramebuffer); {
        renderDevice->clear(true, false, false);
    } renderDevice->pop2D();

    m_scene = Scene::create();
    
    GuiTheme::Ref theme = debugWindow->theme();

    GuiWindow::Ref toolBar = GuiWindow::create("Tools", theme, Rect2D::xywh(0,0,0,0), GuiTheme::TOOL_WINDOW_STYLE);

    IconSet::Ref icons = IconSet::fromFile(System::findDataFile("tango.icn"));
    GuiPane* toolPane = toolBar->pane();

    toolPane->addButton(icons->get("22x22/uwe/CreateCylinder.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/CreateBox.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/Emitter.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/uwe/PointLight.png"), GuiTheme::TOOL_BUTTON_STYLE)->moveBy(Vector2(10,0));
    toolPane->addButton(icons->get("22x22/categories/applications-multimedia.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-graphics.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolPane->addButton(icons->get("22x22/categories/applications-system.png"), GuiTheme::TOOL_BUTTON_STYLE);
    toolBar->pack();
    addWidget(toolBar);

    
}


void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    GApp::onPose(surfaceArray, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
    if (m_scene.notNull()) {
        m_scene->onPose(surfaceArray);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) { 
    // Render full shading viewport
    Rect2D shadeViewport = Rect2D::xywh(0, 0, rd->width() / 2, rd->height() / 2);
    rd->setViewport(shadeViewport);
    Draw::skyBox(rd, m_scene->lighting()->environmentMapTexture, m_scene->lighting()->environmentMapConstant);
    Surface::renderDepthOnly(rd, surface3D, RenderDevice::CULL_BACK);

    if(GLCaps::supportsSAO()){
        rd->push2D(m_aoFramebuffer); {
            m_sao->compute(rd, m_depthBuffer, defaultCamera);
        } rd->pop2D();
    }

    m_scene->lighting()->ambientOcclusion = m_aoTexture;

    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);

    // Wireframe views
    GCamera wireCamera[3];
    wireCamera[0].setCoordinateFrame(CFrame::fromXYZYPRDegrees(0,40,0,0,-90));
    wireCamera[1].setCoordinateFrame(CFrame::fromXYZYPRDegrees(0,0,40,0,0));
    wireCamera[2].setCoordinateFrame(CFrame::fromXYZYPRDegrees(40,0,0,90,0));

    Rect2D wireViewport[3];
    wireViewport[0] = shadeViewport + Vector2(rd->width() / 2, 0.0f);
    wireViewport[1] = shadeViewport + Vector2(rd->width() / 2, rd->height() / 2);
    wireViewport[2] = shadeViewport + Vector2(0.0f, rd->height() / 2);

    for (int i = 0; i < 3; ++i) {
        rd->setViewport(wireViewport[i]);
        rd->setProjectionAndCameraMatrix(wireCamera[i]);

        Surface::renderWireframe(rd, surface3D);
        Draw::axes(rd);
        // Draw::lighting(m_scene->lighting(), rd, true);

        // Call to make the GApp show the output of debugDraw calls
        drawDebugShapes();
    }
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    rd->setColor(Color3::black());
    rd->beginPrimitive(PrimitiveType::LINES); {
        rd->sendVertex(Point2(rd->width() / 2.0f, 0.0f)); 
        rd->sendVertex(Point2(rd->width() / 2.0f, rd->height())); 
        rd->sendVertex(Point2(0.0f, rd->height() / 2.0f)); 
        rd->sendVertex(Point2(rd->width(), rd->height() / 2.0f)); 
    } rd->endPrimitive();

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
