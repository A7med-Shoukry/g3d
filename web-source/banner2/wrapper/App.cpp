/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1680; 
    settings.window.height      = 600;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    showRenderingStats = false;
    m_shadowMap = ShadowMap::create();

    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(0, -1.7f, 15.6f, 360.0f, 10.4f));
    m_film->setGamma(1.0f);
    defaultCamera.setFieldOfView(90 * units::degrees(), GCamera::HORIZONTAL);

    m_model = G3D::ArticulatedModel::createHeightfield(Image1::createEmpty(89, 11), 1.0f);
    m_model->partArray[0].triList[0]->material = Material::createDiffuse("ring-texture.png");

    // Roll up into a ring
    static const float arclength = 150 * units::degrees();
    static const float radius = 20.0f;
    Array<Vector3>& vertex = m_model->partArray[0].geometry.vertexArray;
    for (int v = 0; v < vertex.size(); ++v) {
        Vector3& P = vertex[v];
        const float theta = P.x * arclength;
        P.x = sin(theta) * radius;
        P.y = -P.z  * radius * 0.25f;
        P.z = -cos(theta) * radius;           
    }
        
    m_model->updateAll();

    m_lighting = Lighting::create();
    m_lighting->ambientTop = Color3::white();
    m_lighting->ambientBottom = Color3::white();

}

void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    // Append any models to the arrays that you want to later be rendered by onGraphics()
    m_model->pose(surfaceArray);
    (void)surface2D;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    Framebuffer::Ref f = Framebuffer::create("Framebuffer");
    Texture::Ref t = Texture::createEmpty("", 6000, 1500, ImageFormat::RGB8());
    f->set(Framebuffer::COLOR0, t);

    rd->pushState(f);
    rd->clear();
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_lighting);
    rd->popState();

    t->toImage3uint8()->save("result.png");
    exit(0);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
