/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    settings.window.defaultIconFilename = "icon.png";

#   ifdef G3D_WIN32
	if (FileSystem::exists("data-files", false)) {
            // Running on Windows, building inside the starter directory
            chdir("data-files");
        } else if (FileSystem::exists("../scratch/michael/data-files", false)) {
            // Running on Windows, building from the G3D.sln project (TODO: remove this from your program!)
            chdir("../scratch/michael/data-files");
        }
#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
}


void App::onInit() {
    GApp::onInit();
    // Called before the application loop begins.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.

    showRenderingStats    = false;
    m_showLightSources    = true;
    m_showAxes            = true;
    m_showWireframe       = false;
    m_preventEntityDrag   = false;
    m_preventEntitySelect = false;

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);


    m_shader2TestTexture = Texture::createEmpty("CT", 512, 512, ImageFormat::RGB32F(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    m_depthBuffer  = Texture::createEmpty("coordDepthBuffer", 512, 512, ImageFormat::DEPTH32F(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    m_testFrameBuffer	 = Framebuffer::create("Test");
    m_testFrameBuffer->set(Framebuffer::COLOR0, m_shader2TestTexture);
    m_testFrameBuffer->set(Framebuffer::DEPTH, m_depthBuffer);
    
    makeGUI();

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    m_shadowMap = ShadowMap::create();

    loadScene();
    Image3::Ref im0 = Image3::fromFile("sineNoise1.jpg");

    m_testTexture0 = Texture::fromImage("Noise", im0);


    Image3::Ref im1 = Image3::fromFile("text-only-logos.png");

    m_testTexture1 = Texture::fromImage("logos", im1);

    m_shader2 = Shader2::fromFiles("test.pix","test.vrt");
}


void App::makeGUI() {
    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);


    GFont::Ref iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    
    // Create a scene management GUI
    GuiPane* scenePane = debugPane->addPane("Scene", GuiTheme::ORNATE_PANE_STYLE);
    scenePane->moveBy(0, -10);
    scenePane->beginRow(); {
        // Example of using a callback; you can also listen for events in onEvent or bind controls to data
        m_sceneDropDownList = scenePane->addDropDownList("", Scene::sceneNames(), NULL, GuiControl::Callback(this, &App::loadScene));

        static const char* reloadIcon = "q";
        static const char* diskIcon = "\xcd";

        scenePane->addButton(GuiText(reloadIcon, iconFont, 14), this, &App::loadScene, GuiTheme::TOOL_BUTTON_STYLE)->setWidth(32);
        scenePane->addButton(GuiText(diskIcon, iconFont, 18), this, &App::saveScene, GuiTheme::TOOL_BUTTON_STYLE)->setWidth(32);
    } scenePane->endRow();

    const int w = 120;
    scenePane->beginRow(); {
        scenePane->addCheckBox("Axes", &m_showAxes)->setWidth(w);
        scenePane->addCheckBox("Light sources", &m_showLightSources);
    } scenePane->endRow();
    scenePane->beginRow(); {
        scenePane->addCheckBox("Wireframe", &m_showWireframe)->setWidth(w);
    } scenePane->endRow();
    static const char* lockIcon = "\xcf";
    scenePane->addCheckBox(GuiText(lockIcon, iconFont, 20), &m_preventEntityDrag, GuiTheme::TOOL_CHECK_BOX_STYLE);
    scenePane->pack();

    GuiPane* entityPane = debugPane->addPane("Entity", GuiTheme::ORNATE_PANE_STYLE);
    entityPane->moveRightOf(scenePane);
    entityPane->moveBy(10, 0);
    m_entityList = entityPane->addDropDownList("Name");

    // Dock the spline editor
    m_splineEditor = PhysicsFrameSplineEditor::create("Spline Editor", entityPane);
    addWidget(m_splineEditor);
    developerWindow->cameraControlWindow->moveTo(Point2(window()->width() - developerWindow->cameraControlWindow->rect().width(), 0));
    m_splineEditor->moveTo(developerWindow->cameraControlWindow->rect().x0y0() - Vector2(m_splineEditor->rect().width(), 0));
    entityPane->pack();

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->moveRightOf(entityPane);
    infoPane->moveBy(10, 0);

    infoPane->addTextureBox(m_shader2TestTexture);
    infoPane->addButton("Reload Shader 2", this, &App::reloadShader2);
    infoPane->addButton("Exit", this, &App::endProgram);
    infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, window()->width(), debugWindow->rect().height()));
}
void App::reloadShader2(){
    m_shader2->reload();
}
void App::testShader2(){
    Array<Surface::Ref> surface3D;
    m_scene->onPose(surface3D);
    debugAssertGLOk();
    
    Args args;
    args.setUniform("jump", 5);
    args.setUniform("noise", m_testTexture1);
    args.setUniform("screenSize", Vector2(512.0f, 512.0f)); 
    args.setMacro("ODD_COLOR", Color3(0.8,0.9,0.2));
    args.setPreamble("#define EVEN_COLOR vec3(0.0, 0.0, 0.4)");
    renderDevice->pushState();{
        renderDevice->setColorClearValue(Color4(1.0,1.0,1.0,1.0));
        renderDevice->clear();
    } renderDevice->popState();
    for(int i = 0; i < surface3D.size(); ++i){
        const SuperSurface::Ref&  surface = surface3D[i].downcast<SuperSurface>();
        if(surface.isNull()) {
            debugPrintf("Surface %d, not a supersurface.\n", i);
            continue;
        }
        const SuperSurface::GPUGeom::Ref& gpuGeom = surface->gpuGeom();
        
        args.setStream("g3d_Normal", gpuGeom->normal);
        args.setStream("g3d_MultiTexCoord0", gpuGeom->texCoord0);
        args.setStream("g3d_Vertex", gpuGeom->vertex);
        args.setIndexArray(gpuGeom->index);
        args.geometryInput = gpuGeom->primitive;
        debugAssertGLOk();
        renderDevice->pushState();{
            renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
            renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            renderDevice->setProjectionAndCameraMatrix(defaultCamera);
            CoordinateFrame cf;
            debugPrintf("Surface %d: %s\n", i, surface->name().c_str());

            surface->getCoordinateFrame(cf);
            debugPrintf("CoordinateFrame:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", 
                cf.rotation[0][0], cf.rotation[0][1], cf.rotation[0][2], cf.translation[0],
                cf.rotation[1][0], cf.rotation[1][1], cf.rotation[1][2], cf.translation[1],
                cf.rotation[2][0], cf.rotation[2][1], cf.rotation[2][2], cf.translation[2]);
            const CoordinateFrame o2w = renderDevice->objectToWorldMatrix();

            renderDevice->setObjectToWorldMatrix(cf);
            debugAssertGLOk();
            renderDevice->apply(m_shader2, args);
            renderDevice->setObjectToWorldMatrix(o2w);
            debugAssertGLOk();
        }renderDevice->popState();
    }

    
}


void App::loadScene() {
    const std::string& sceneName = m_sceneDropDownList->selectedValue().text();

    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    // Load the scene
    try {
        m_scene = Scene::create(sceneName, defaultCamera);
        defaultController->setFrame(defaultCamera.coordinateFrame());

        // Populate the entity list
        Array<std::string> nameList;
        m_scene->getEntityNames(nameList);
        m_entityList->clear();
        m_entityList->append("<none>");
        for (int i = 0; i < nameList.size(); ++i) {
            m_entityList->append(nameList[i]);
        }

    } catch (const ParseError& e) {
        const std::string& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
        drawMessage(msg);
        debugPrintf("%s", msg.c_str());
        System::sleep(5);
        m_scene = NULL;
    }
}


void App::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    m_splineEditor->setEnabled(m_splineEditor->enabled() && ! m_preventEntityDrag);
	
    m_splineEditor->setVisible(m_splineEditor->enabled());

    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.
    if (m_scene.notNull()) {
        if (m_selectedEntity.notNull() && m_splineEditor->enabled()) {
            // Apply the edited spline.  Do this before object simulation, so that the object
            // is in sync with the widget for manipulating it.
            m_selectedEntity->setFrameSpline(m_splineEditor->spline());
        }

        m_scene->onSimulation(sdt);
    }
}


bool App::onEvent(const GEvent& event) {
    if (GApp::onEvent(event)) {
        return true;
    }

    if (event.type == GEventType::VIDEO_RESIZE) {
        // Example GUI dynamic layout code.  Resize the debugWindow to fill
        // the screen horizontally.
        debugWindow->setRect(Rect2D::xywh(0, 0, window()->width(), debugWindow->rect().height()));
    }


    if (! m_preventEntitySelect && (event.type == GEventType::MOUSE_BUTTON_DOWN) && (event.button.button == 0)) {
        // Left click: select by casting a ray through the center of the pixel
        const Ray& ray = defaultCamera.worldRay(event.button.x + 0.5f, event.button.y + 0.5f, renderDevice->viewport());
        
        float distance = finf();

        selectEntity(m_scene->intersect(ray, distance));
    }
    

    if (! m_preventEntitySelect && (event.type == GEventType::GUI_ACTION) && (event.gui.control == m_entityList)) {
        // User clicked on dropdown list
        selectEntity(m_scene->entity(m_entityList->selectedValue().text()));
    }

    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true;}
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }

    return false;
}


void App::selectEntity(const Entity::Ref& e) {
    m_selectedEntity = e;

    if (m_selectedEntity.notNull()) {
        m_splineEditor->setSpline(m_selectedEntity->frameSpline());
        m_splineEditor->setEnabled(! m_preventEntityDrag);
        m_entityList->setSelectedValue(m_selectedEntity->name());
    } else {
        m_splineEditor->setEnabled(false);
    }
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    GApp::onPose(posed3D, posed2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
    if (m_scene.notNull()) {
        m_scene->onPose(posed3D);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    if (m_scene.isNull()) {
        return;
    }
    testShader2();
    /*Draw::skyBox(rd, m_scene->skyBoxTexture(), m_scene->skyBoxConstant());

    // Render all objects (or, you can call Surface methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);
    
    
    if (m_showWireframe) {
        Surface::renderWireframe(rd, surface3D);
    }

    //////////////////////////////////////////////////////
    // Sample immediate-mode rendering code
    
    rd->enableLighting();
    for (int i = 0; i < m_scene->lighting()->lightArray.size(); ++i) {
        rd->setLight(i, m_scene->lighting()->lightArray[i]);
    }
    rd->setAmbientLightColor(Color3::white() * 0.5f);

    //Draw::sphere(Sphere(Vector3(2.5f, 0.5f, 0), 0.5f), rd, Color3::white(), Color4::clear());
    //Draw::box(AABox(Vector3(-2.0f, 0.0f, -0.5f), Vector3(-1.0f, 1.0f, 0.5f)), rd, Color4(Color3::orange(), 0.25f), Color3::black());

    if (m_showAxes) {
        Draw::axes(Point3(0, 0, 0), rd);
    }    

    if (m_showLightSources) {
        Draw::lighting(m_scene->lighting(), rd);
    }
    
    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();*/
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}


void App::endProgram() {
    m_endProgram = true;
}


void App::saveScene() {
    // Called when the "save" button is pressed
    if (m_scene.notNull()) {
        Any a = m_scene->toAny();
        const std::string& filename = a.source().filename;
        if (filename != "") {
            a.save(filename);
            debugPrintf("Saved %s\n", filename.c_str());
        } else {
            debugPrintf("Could not save: empty filename");
        }
    }
}
