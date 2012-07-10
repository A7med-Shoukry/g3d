#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);
    
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    settings.window.caption     = "G3D Deferred Shading Sample";


#   ifdef G3D_WIN32
	if (! FileSystem::exists("deferred.pix", false)) {
        // Running on Windows, building from the G3D.sln project
        chdir("../samples/deferredShading");
    }
#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    makeGBuffer();
    makeScene();
    makeShader();
    makeGUI();
    setDesiredFrameRate(60);
}


void App::makeGBuffer() {
    GBuffer::Specification specification;
    specification.format[GBuffer::Field::WS_NORMAL]          = ImageFormat::RGB16F();
    specification.format[GBuffer::Field::SS_POSITION_CHANGE] = ImageFormat::RG16F();
    specification.format[GBuffer::Field::WS_POSITION]        = ImageFormat::RGB32F();
    specification.format[GBuffer::Field::LAMBERTIAN]         = ImageFormat::RGB8();
    specification.format[GBuffer::Field::GLOSSY]             = ImageFormat::RGBA8();
    specification.format[GBuffer::Field::DEPTH_AND_STENCIL]  = ImageFormat::DEPTH24();
    specification.depthEncoding = DepthEncoding::HYPERBOLIC;

    gbuffer = GBuffer::create(specification);

    gbuffer->resize(renderDevice->width(), renderDevice->height());

    // Share the depth buffer with the forward-rendering pipeline
    m_depthBuffer = gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
    m_frameBuffer->set(Framebuffer::DEPTH, m_depthBuffer);
}


void App::makeScene() {
    renderDevice->setColorClearValue(Color3::white() * 0.5f);

    defaultCamera.setFarPlaneZ(-20.0f);
    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(  1.5f,   0.2f,   1.8f,  42.4f,   3.4f,   0.0f));

    m_film->setAntialiasingEnabled(true);

    model = 
        ArticulatedModel::create(PARSE_ANY(
         ArticulatedModel::Specification {            
            filename = "ifs/crate.ifs";
            preprocess = (
                          setMaterial(all(), all(), #include("material/metalcrate/metalcrate.mat.any"));
                          );
         }));
}


void App::makeShader() {
    shadingPass = Shader::fromFiles("", "deferred.pix");
}


void App::makeGUI() {
    debugWindow->setVisible(true);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;

    // Show the G-buffers
    debugPane->setCaption(GuiText("G-Buffers", GFont::fromFile(System::findDataFile("arial.fnt")), 16));
    debugPane->moveBy(2, 10);

    GuiTabPane* tabPane = debugPane->addTabPane();
    debugPane->beginRow();
    for (int i = 0; i < GBuffer::Field::COUNT; ++i) {
        GBuffer::Field f = (GBuffer::Field)i;
        Texture::Ref t = gbuffer->texture(f);

        if (t.notNull()) {
            GuiPane* p = tabPane->addTab(f.toString());
            p->addTextureBox(t);
        }
    }
    tabPane->pack();
    tabPane->setWidth(800);
    debugPane->pack();
}


void App::onPose(Array<Surface::Ref>& surface, Array<Surface2D::Ref>& surface2D) {
    GApp::onPose(surface, surface2D);
    static float yaw = 150.0f * units::degrees();
    
    const CFrame& previousFrame = CFrame::fromXYZYPRDegrees(0,0,0,yaw,0,0);

    yaw += 20.0f * units::degrees();
    const CFrame& currentFrame = CFrame::fromXYZYPRDegrees(0,0,0,yaw,0,0);

    model->pose(surface, currentFrame, ArticulatedModel::defaultPose(), previousFrame, ArticulatedModel::defaultPose());
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {

    // Generate the gbuffer
    gbuffer->prepare(rd, defaultCamera, 0, -1.0f / desiredFrameRate());
    Array<Surface::Ref> visibleArray;
    Surface::cull(defaultCamera, rd->viewport(), surface3D, visibleArray);
    glDisable(GL_DEPTH_CLAMP);
    Surface::renderIntoGBuffer(rd, visibleArray, gbuffer, previousCameraFrame);
    previousCameraFrame = defaultCamera.coordinateFrame();

    // Make a pass over the screen, performing shading
    rd->push2D(); {
        shadingPass->args.set("wsEye",              gbuffer->camera().coordinateFrame().translation);
        gbuffer->bindReadUniforms(shadingPass->args);
 
        rd->applyRect(shadingPass);
    } rd->pop2D();

    // Forward-render other objects
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
