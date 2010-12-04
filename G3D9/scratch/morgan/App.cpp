/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

/** 
 CubeMap<Image3>::Ref cube = CubeMap<Image3>::create(...);
*/
template<class Image>
class CubeMap : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer< class CubeMap<Image> > Ref;

protected:

    int                         m_width;
    Array<typename Image::Ref>     m_imageArray;

    CubeMap() {}

    void init(const Array<typename Image::Ref>& imageArray) {
        m_imageArray = imageArray;
        debugAssert(imageArray.size() == 6);
        m_width = m_imageArray[0]->width();
        for (int i = 0; i < 6; ++i) {
            debugAssert(m_imageArray[i]->width() == m_width);
            debugAssert(m_imageArray[i]->height() == m_width);
        }    
    }

public:

    static Ref create(const std::string& fileSpec) {
        // TODO
        return NULL;
    }

    static Ref create(const Array<typename Image::Ref>& imageArray) {
        Ref c = new CubeMap<Image>();
        c->init(imageArray);
        return c;
    }

    typename Image::ComputeType bilinear(const Vector3& v) const;

    typename Image::ComputeType nearest(const Vector3& v) const;

    /** Return the image representing one face. */
    const typename Image::Ref face(CubeFace f) const {
        return m_imageArray[f];
    }
    
    /** Returns the width of one side, which must be the same as the height. */
    int width() const {
        return m_width;
    }

    /** Returns the height of one side in pixels, which must be the same as the height. */
    int height() const {
        // Width and height are the same in this class.
        return m_width;
    }

};

int main(int argc, char** argv) {

#if 0
    Image1::Ref im = Image1::createEmpty(32, 64);
    for (int y = 0; y < im->height(); ++y) {
        for (int x = 0; x < im->width(); ++x) {
            im->set(x, y, Color1(x / float(im->width() - 1)));
        }
    }
    im->save("test.png", GImage::PNG16);
    debugPrintf("Done\n");
    return 0;
#endif

    (void)argc; (void)argv;
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 960; 
    settings.window.height      = 600;
    settings.window.stencilBits = 8;

    settings.film.preferredDepthFormats.clear();
//    settings.film.preferredDepthFormats.append(ImageFormat::DEPTH24());
    settings.film.preferredDepthFormats.append(ImageFormat::DEPTH24_STENCIL8());
//    settings.film.enabled = false;

#   ifdef G3D_WIN32
	if (FileSystem::exists("data-files", false)) {
            // Running on Windows, building inside the starter directory
            chdir("data-files");
        } else if (FileSystem::exists("../samples/starter/data-files", false)) {
            // Running on Windows, building from the G3D.sln project (TODO: remove this from your program!)
            chdir("../samples/starter/data-files");
        }
#   endif

        //CubeMap<Image3>::Ref im = CubeMap<Image3>::create(System::findDataFile("test/testcube_*.jpg"));
    

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
}


void App::onInit() {
    // Called before the application loop beings.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.
    
    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);
    showRenderingStats = true;


    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    // Example of using a callback; you can also listen for events in onEvent or bind controls to data
    m_sceneDropDownList = debugPane->addDropDownList("Scene", Scene::sceneNames(), NULL, GuiControl::Callback(this, &App::loadScene));
    debugPane->addButton(GuiText("q", GFont::fromFile(System::findDataFile("icon.fnt")), 14), this, &App::loadScene, GuiTheme::TOOL_BUTTON_STYLE)->moveRightOf(m_sceneDropDownList);
    debugWindow->pack();
    debugWindow->moveTo(Vector2(0, window()->height() - debugWindow->rect().height()));

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    m_shadowMap = ShadowMap::create();

//    loadScene();
    
    //show(Image3::createEmpty(600, 300));
    //show(Image3::createEmpty(600, 300));
}


void App::loadScene() {
    const std::string& sceneName = m_sceneDropDownList->selectedValue().text();

    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    // Load the scene
    m_scene = Scene::create(sceneName, defaultCamera);
}


void App::onAI() {
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt; (void)sdt; (void)rdt;
    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.
//    m_scene->onSimulation(sdt);
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }
    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((e.type == GEventType::GUI_ACTION) && (e.gui.control == m_button)) { ... return true;}
    // if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::TAB)) { ... return true; }

    return false;
}


void App::onUserInput(UserInput* ui) {
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    // Append any models to the arrays that you want to later be rendered by onGraphics()
//    m_scene->onPose(surfaceArray);
    (void)surface2D;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
/*    if (m_scene->lighting()->environmentMapTexture.notNull()) {
        Draw::skyBox(rd, m_scene->lighting()->environmentMapTexture, m_scene->lighting()->environmentMapConstant);
    }

    // Render all objects (or, you can call Surface methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);
    */

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
    
    rd->push2D();
    {
        rd->setColorClearValue(Color3::white());
        //rd->setStencilClearValue(0);    
        rd->clear(true, true, true);

        rd->setStencilConstant(1);
        rd->setStencilOp(RenderDevice::STENCIL_KEEP, RenderDevice::STENCIL_KEEP, RenderDevice::STENCIL_REPLACE);
        Draw::rect2D(Rect2D::xywh(100, 100, 100, 100), rd, Color3::red());

        rd->setStencilOp(RenderDevice::STENCIL_KEEP, RenderDevice::STENCIL_KEEP, RenderDevice::STENCIL_KEEP);
        rd->setStencilTest(RenderDevice::STENCIL_EQUAL);
        Draw::rect2D(rd->viewport(), rd, Color3::blue());
    }
    rd->pop2D();

    
    /*
    m_shader->args.set("cubemap", m_scene->lighting()->environmentMapTexture);
    rd->setShader(m_shader);
    Array<Surface::Ref> a;
    m_sphere->pose(a);
    a[0]->sendGeometry(rd);
    */

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
