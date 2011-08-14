/**
  @file App.cpp
 */
#include "App.h"
#include "World.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.caption     = "G3D Ray Trace Sample";
    settings.window.width       = 960; 
    settings.window.height      = 640;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : 
    GApp(settings),
    m_maxBounces(3), 
    m_raysPerPixel(1), 
    m_world(NULL) {
    catchCommonExceptions = false;
}


void App::onInit() {
    message("Loading...");
    m_world = new World();

    showRenderingStats = false;
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    m_film->setAntialiasingEnabled(true);

    // Starting position
    defaultCamera.setCoordinateFrame(CFrame::fromXYZYPRDegrees(24.3f, 0.4f, 2.5f, 68.7f, 1.2f, 0.0f));
    m_prevCFrame = defaultCamera.coordinateFrame();

    makeGUI();
    onRender();
}


void App::makeGUI() {
    GuiWindow::Ref window = GuiWindow::create("Controls", debugWindow->theme(), Rect2D::xywh(0,0,0,0), GuiTheme::TOOL_WINDOW_STYLE);
    GuiPane* pane = window->pane();
    pane->addLabel("Use WASD keys + right mouse to move");
    pane->addButton("Render High Res.", this, &App::onRender);
    
    pane->addNumberBox("Rays per pixel", &m_raysPerPixel, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);
    pane->addNumberBox("Max bounces", &m_maxBounces, "", GuiTheme::LINEAR_SLIDER, 1, 16, 1);
    window->pack();

    window->setVisible(true);
    addWidget(window);
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& surface3D, Array<Surface2D::Ref>& surface2D) {
    // Update the preview image only while moving
    if (! m_prevCFrame.fuzzyEq(defaultCamera.coordinateFrame())) {
        rayTraceImage(0.2f, 1);
        m_prevCFrame = defaultCamera.coordinateFrame();
    }

    rd->clear();

    if (m_result.notNull()) {
        rd->push2D();
        rd->setTexture(0, m_result);
        Draw::rect2D(rd->viewport(), rd);
        rd->pop2D();
    }

    Surface2D::sortAndRender(rd, surface2D);
}


void App::onCleanup() {
    delete m_world;
    m_world = NULL;
}


static G3D::Random rnd(0xF018A4D2, false);

Radiance3 App::rayTrace(const Ray& ray, World* world, int bounce) {
    Radiance3 radiance = Radiance3::zero();
    const float BUMP_DISTANCE = 0.0001f;

    SurfaceElement surfel;
    float dist = inf();
    if (world->intersect(ray, dist, surfel)) {
        // Shade this point (direct illumination)
        for (int L = 0; L < world->lightArray.size(); ++L) {
            const GLight& light = world->lightArray[L];

            // Shadow rays
            if (world->lineOfSight(surfel.geometric.location + surfel.geometric.normal * BUMP_DISTANCE, light.position.xyz())) {
                Vector3 w_i = light.position.xyz() - surfel.shading.location;
                const float distance2 = w_i.squaredLength();
                w_i /= sqrt(distance2);

                // Attenduated radiance
                const Irradiance3& E_i = light.color / (4.0f * pif() * distance2);

                radiance += 
                    surfel.evaluateBSDF(w_i, -ray.direction()) * 
                    E_i *
                    max(0.0f, w_i.dot(surfel.shading.normal));

                debugAssert(radiance.isFinite());
            }
        }

        // Indirect illumination
        // Ambient
        radiance += surfel.material.lambertianReflect * world->ambient;

        // Specular
        if (bounce < m_maxBounces) {
            // Perfect reflection and refraction
            SmallArray<SurfaceElement::Impulse, 3> impulseArray;
            surfel.getBSDFImpulses(-ray.direction(), impulseArray);
                
            for (int i = 0; i < impulseArray.size(); ++i) {
                const SurfaceElement::Impulse& impulse = impulseArray[i];
                // Bump along the ray direction, which may be into the surface
                const Vector3& offset = impulse.w * sign(impulse.w.dot(surfel.geometric.normal)) * BUMP_DISTANCE;
                const Ray& secondaryRay = Ray::fromOriginAndDirection(surfel.geometric.location + offset, impulse.w);
				debugAssert(secondaryRay.direction().isFinite());
                radiance += rayTrace(secondaryRay, world, bounce + 1) * impulse.magnitude;
				debugAssert(radiance.isFinite());
            }
        }
    } else {
        // Hit the sky
        radiance = world->ambient;
    }

    return radiance;
}


void App::message(const std::string& msg) const {
    renderDevice->clear();
    renderDevice->push2D();
        debugFont->draw2D(renderDevice, msg, renderDevice->viewport().center(), 12, 
            Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    renderDevice->pop2D();

    // Force update so that we can see the message
    renderDevice->swapBuffers();
}


void App::onRender() {
    // Show message
    message("Rendering...");

    Stopwatch timer;
    rayTraceImage(1.0f, m_raysPerPixel);
    timer.after("Trace");
    debugPrintf("%f s\n", timer.elapsedTime());
//    m_result->toImage3uint8()->save("result.png");
}


void App::trace(int x, int y) {
    Color3 sum = Color3::black();
    if (m_currentRays == 1) {
        sum = rayTrace(defaultCamera.worldRay(x + 0.5f, y + 0.5f, m_currentImage->rect2DBounds()), m_world);
    } else {
        for (int i = 0; i < m_currentRays; ++i) {
            sum += rayTrace(defaultCamera.worldRay(x + rnd.uniform(), y + rnd.uniform(), m_currentImage->rect2DBounds()), m_world);
        }
    }
    m_currentImage->set(x, y, sum / m_currentRays);
}


void App::rayTraceImage(float scale, int numRays) {

    int width  = window()->width()  * scale;
    int height = window()->height() * scale;
    
    if (m_currentImage.isNull() || (m_currentImage->width() != width) || (m_currentImage->height() != height)) {
        m_currentImage = Image3::createEmpty(width, height);
    }
    m_currentRays = numRays;
    GThread::runConcurrently2D(Point2int32(0, 0), Point2int32(width, height), this, &App::trace);

    // Post-process
    Texture::Ref src = Texture::fromImage("Source", m_currentImage);
    if (m_result.notNull()) {
        m_result->resize(width, height);
    }
    m_film->exposeAndRender(renderDevice, src, m_result);
}
