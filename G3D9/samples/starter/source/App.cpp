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
        } else if (FileSystem::exists("../samples/starter/data-files", false)) {
            // Running on Windows, building from the G3D.sln project (TODO: remove this from your program!)
            chdir("../samples/starter/data-files");
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

Color3unorm8 radiance3tosRGB8(const Radiance3& radiance){
        //return Color3unorm8(radiance);
	/* TODO: Move this someplace sensible.
		Set this to be the inverse of the maximum 
		radiance value expected */
        static const float inverseMaxRadiance = 1.0f;
        
        static bool init = true;
#define POW_CACHE_SIZE (10000)
        static unorm8 powInvTwoPointTwo[POW_CACHE_SIZE];
        if(init){
            init = false;
            for(int i = 0; i < POW_CACHE_SIZE; ++i){
                float f =  ::pow(float(i)/(POW_CACHE_SIZE-1.0f), (1/2.2f));
                powInvTwoPointTwo[i] = unorm8(f);
            }
        }
        const Radiance3 normalizedRadiance = radiance * inverseMaxRadiance;
        //if(normalizedRadiance.max() >= 1.0f) debugPrintf("Normalized Radiance too large! Use something bigger than %f\n", normalizedRadiance.max());
        
        Color3unorm8 c;
        c.r = powInvTwoPointTwo[int(normalizedRadiance.r * POW_CACHE_SIZE)];
        c.g = powInvTwoPointTwo[int(normalizedRadiance.g * POW_CACHE_SIZE)];
        c.b = powInvTwoPointTwo[int(normalizedRadiance.b * POW_CACHE_SIZE)];
        return c;
	    
	    //return Color3unorm8((radiance * inverseMaxRadiance).pow(1/2.2f));
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

    Image3::Ref im = Image3::fromFile("test.png");
    m_texture = Texture::createEmpty("E",im->width(), im->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT);
    m_texture->clear();
    int imageByteNum = im->width() * im->height() * 3;

    GLuint pbo = GL_NONE;
    glGenBuffers(1, &pbo);
    debugAssertGLOk();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    debugAssertGLOk();
    // allocate storage for PBO
    glBufferData(GL_PIXEL_UNPACK_BUFFER, imageByteNum, 0, GL_STREAM_DRAW);
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
    debugAssertGLOk();


  
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    debugAssertGLOk();
    void *pboBuffer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, imageByteNum, GL_MAP_WRITE_BIT);
    debugAssertGLOk();
    alwaysAssertM(pboBuffer != NULL, "Pixel Buffer NULL.\n");
    Color3unorm8* gpuPixelArray = (Color3unorm8 *)pboBuffer;

    Color3* cpuPixelArray = im->getCArray();
    int pixelNum = im->height() * im->width();
    for (int i = 0; i < pixelNum; ++i) {
        gpuPixelArray[i] = radiance3tosRGB8(cpuPixelArray[i]);
    }
    pboBuffer = NULL;
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    debugAssertGLOk();

    glPushAttrib(GL_TEXTURE_BIT);
    {
        glBindTexture(m_texture->openGLTextureTarget(), m_texture->openGLID());
        debugAssertGLOk();

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        debugAssertGLOk();
            
        const GLint xoffset = 0;
        const GLint yoffset = 0;
        const GLint mipLevel = 0;
        GLenum target = m_texture->openGLTextureTarget();

        glTexSubImage2D
                (target, 
                    mipLevel,
                    xoffset,
                    yoffset,
                    im->width(), 
                    im->height(), 
                    ImageFormat::RGB8()->openGLBaseFormat,
                    ImageFormat::RGB8()->openGLDataFormat, 
                    0);
        debugAssertGLOk();
        glBindTexture(m_texture->openGLTextureTarget(), GL_NONE);
    } glPopAttrib();
    debugAssertGLOk();
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_NONE);
    debugAssertGLOk();

    
    debugAssertGLOk();

    glDeleteBuffers(1, &pbo);
    debugAssertGLOk();
    pbo = GL_NONE;

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    m_shadowMap = ShadowMap::create();
    loadScene();
    makeGUI();

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    

  
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

    // Example of how to add debugging controls
    infoPane->addLabel("You can add more GUI controls");
    infoPane->addLabel("in App::onInit().");
    infoPane->addButton("Exit", this, &App::endProgram);
    //infoPane->addTextureBox(m_scene->m_skyBoxTexture);
    infoPane->addTextureBox(m_texture);
    infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, window()->width(), debugWindow->rect().height()));
}


void App::loadScene() {
    const std::string& sceneName = "Crates";//m_sceneDropDownList->selectedValue().text();

    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    // Load the scene
    try {
        m_scene = Scene::create(sceneName, defaultCamera);
        defaultController->setFrame(defaultCamera.coordinateFrame());

        // Populate the entity list
        /*Array<std::string> nameList;
        m_scene->getEntityNames(nameList);
        m_entityList->clear();
        m_entityList->append("<none>");
        for (int i = 0; i < nameList.size(); ++i) {
            m_entityList->append(nameList[i]);
        }*/

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

        m_scene->onSimulation(sdt/10);
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
    Draw::skyBox(rd, m_scene->skyBoxTexture(), m_scene->skyBoxConstant());

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

    Draw::sphere(Sphere(Vector3(2.5f, 0.5f, 0), 0.5f), rd, Color3::white(), Color4::clear());
    Draw::box(AABox(Vector3(-2.0f, 0.0f, -0.5f), Vector3(-1.0f, 1.0f, 0.5f)), rd, Color4(Color3::orange(), 0.25f), Color3::black());

    if (m_showAxes) {
        Draw::axes(Point3(0, 0, 0), rd);
    }    

    if (m_showLightSources) {
        Draw::lighting(m_scene->lighting(), rd);
    }

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
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
