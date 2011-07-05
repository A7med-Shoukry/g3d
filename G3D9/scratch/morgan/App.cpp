/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1280; 
    settings.window.height      = 720;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    renderDevice->setColorClearValue(Color3::white());
}


void App::onInit() {

    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;

    std::string materialPath = System::findDataFile("material");
    std::string crateFile = System::findDataFile("crate.ifs");
    model = ArticulatedModel::fromFile(crateFile);
    /*
    Material::Specification mat;
    std::string base = pathConcat(materialPath, "metalcrate/metalcrate-");
    mat.setLambertian(base + "L.png", 0.2f);
    mat.setSpecular(base + "G.png");
    mat.setGlossyExponentShininess(20);
    BumpMap::Settings b;
    b.iterations = 1;
    mat.setBump(base + "B.png", b);
    Material::Ref material = Material::create(mat);

    */
    /*
    // Save material
    {
        BinaryOutput b("material.mat.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        material->speedSerialize(sid, b);
        b.commit();
    }

    // Load material
    {
        BinaryInput b("material.mat.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        material = Material::speedCreate(sid, b);
    }*/

//    model->partArray[0].triList[0]->material = material;

    // Save Model
    { 
        BinaryOutput b("model.am.sl", G3D_LITTLE_ENDIAN);
        model->speedSerialize(b);
        b.commit();
    }

    // Load Model
    {
        BinaryInput b("model.am.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        model = ArticulatedModel::speedCreate(b);
    }

    lighting = defaultLighting();
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

void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);

    model->pose(surface3D);
    Surface::sortAndRender(rd, defaultCamera, surface3D, lighting);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
