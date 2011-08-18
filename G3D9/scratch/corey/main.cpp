
#include <G3D/G3DAll.h>
#include "irrklang/irrKlang.h"
#include "Image.h"
#include "G3D/ImageBuffer.h"

//#pragma comment(lib, "irrKlang.lib") // link with irrKlang.dll
#pragma comment(lib, "freeimaged.lib")


class App : public GApp {
public:
    LightingRef         lighting;
    Texture::Ref        sky;

    MD3Model::Ref       model;
    MD3Model::Pose      modelPose;

    //irrklang::ISoundEngine* irrklangDevice;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onAI();
    virtual void onNetwork();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D);

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);

    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D);

    virtual bool onEvent(const GEvent& e);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();

    /** Sets m_endProgram to true. */
    virtual void endProgram();
};


// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 720; 
    settings.window.height      = 480;
	settings.window.resizable   = true;

#   ifdef G3D_WIN32
        // On unix-like operating systems, icompile automatically
        // copies data files.  On Windows, we just run from the data
        // directory.
		if (FileSystem::exists("data-files")) {
            chdir("data-files");
        }
#   endif

    FreeImage_Initialise();
    Image::Ref img = Image::fromFile("C:\\dev\\G3D\\G3D9\\data-files\\3ds\\spaceFighter01\\diffuse.jpg");
    Color4 c;
    img->get(Point2int32(0, 0), c);
    FreeImage_DeInitialise();

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

    showRenderingStats = true;

    sky = Texture::fromFile(dataDir + "/cubemap/noonclouds/noonclouds_*.png", ImageFormat::AUTO(), Texture::DIM_CUBE_MAP_NPOT, Texture::Settings::cubeMap(), Texture::Preprocess::gamma(2.1f));
    lighting = Lighting::create();

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    
    //GuiTheme::makeThemeFromSourceFiles(pathConcat(dataDir, "skin"), "osx_white.png", "osx_black.png", "osx.txt", pathConcat(dataDir, "osx_new.gtm"));

    //GuiTheme::Ref theme = GuiTheme::fromFile("osx_new.gtm");
    
    MD3Model::Skin::Ref skin = MD3Model::Skin::create(dataDir + "/md3/chaos-marine/models/players/chaos-marine/", "default");

    MD3Model::Specification spec;
    spec.directory = dataDir + "/md3/chaos-marine/models/players/chaos-marine/";
    spec.defaultSkin = skin;

    model = MD3Model::create(spec);

    modelPose.anim[MD3Model::PART_LOWER] = MD3Model::LOWER_WALK;
    modelPose.anim[MD3Model::PART_UPPER] = MD3Model::UPPER_STAND;


	// start the sound engine with default parameters
	//irrklangDevice = irrklang::createIrrKlangDevice();
    //debugAssert(irrklangDevice);
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

    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.

    //irrklangDevice->play2D("test.wmv");

    modelPose.time[MD3Model::PART_LOWER] += sdt;
    modelPose.time[MD3Model::PART_UPPER] += sdt;
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
    GApp::onUserInput(ui);

    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    GApp::onPose(posed3D, posed2D);

    model->pose(posed3D, CoordinateFrame(), modelPose);
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D) {
    // Draw sky
    Draw::skyBox(rd, sky);

    // Render all objects (or, you can call Surface methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)

    Surface::sortAndRender(rd, defaultCamera, posed3D, lighting);

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
	//irrklangDevice->drop();
}


void App::endProgram() {
    m_endProgram = true;
}
