#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);
    
    settings.window.width       = 1280; 
    settings.window.height      = 720;
    settings.window.caption     = "G3D Deferred Shading Sample";

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    makeGBuffer();
    makeScene();
    makeShader();
    makeGUI();
}


void App::makeGBuffer() {
    GBuffer::Specification specification;
    specification.format[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB16F();
    specification.format[GBuffer::Field::WS_POSITION] = ImageFormat::RGB16F();
    specification.format[GBuffer::Field::LAMBERTIAN]  = ImageFormat::RGB8();
    specification.format[GBuffer::Field::GLOSSY]      = ImageFormat::RGBA8();
//    specification.format[GBuffer::Field::WS_FACE_NORMAL]  = ImageFormat::RGBA8();
    specification.format[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH24();
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

    Any crateSpec;
    crateSpec.parse(
        STR(
         ArticulatedModel::Specification {            
            filename = "ifs/crate.ifs";
            preprocess = ArticulatedModel::Preprocess {
                materialOverride = #include("material/metalcrate/metalcrate.mat.any")
            }
        }));
    model = ArticulatedModel::create(crateSpec);
}


void App::makeShader() {

    shadingPass = Shader::fromStrings("",
        std::string("#version 120\n#extension GL_EXT_gpu_shader4 : require\n#define PI 3.1415926\n") +      
        STR(
        uniform sampler2D wsPosition;
        uniform sampler2D wsNormal;
        uniform sampler2D lambertian;
        uniform sampler2D glossy;
        uniform vec3      wsEye;

        // Direction to the light in world space
        const vec3 w_i            = vec3(0.0, 0.71, 0.71);
        const vec3 lightRadiance  = vec3(5.0, 5.0, 4.5);
        const vec3 ambient        = vec3(0.3, 0.4, 0.5);

        void main() {
            // Screen-space point being shaded
            ivec2 C = ivec2(gl_FragCoord.xy);

            // Surface normal
            vec3 n = texelFetch2D(wsNormal, C, 0).xyz;
            if (dot(n, n) < 0.01) {
                // This is a background pixel, not part of an object
                discard;
            } else {
                n = n * 2.0 - 1.0;
            }

            // Lambertian coefficient in BSDF
            vec3 p_L  = texelFetch2D(lambertian, C, 0).rgb / PI;

            // Glossy coefficient in BSDF (this code unpacks G3D::SuperBSDF's encoding)
            vec4 temp = texelFetch2D(glossy, C, 0);
            float exponent = (temp.a * temp.a) * 1024.0 + 1.0;
            vec3 p_G = temp.rgb * ((exponent + 8.0) / (8.0 * PI));

            // Point being shaded
            vec3 X = texelFetch2D(wsPosition, C, 0).xyz;
            
            // View vector
            vec3 w_o = normalize(wsEye - X);

            // Half vector
            vec3 w_h = normalize(w_i + w_o);

            // Energy-conserving Phong shading
            gl_FragColor.rgb =
                ambient * p_L +

                lightRadiance * 
                (p_L + p_G * pow(max(dot(n, w_h), 0.0), exponent)) * max(dot(n, w_i), 0.0);
        }));
}


void App::makeGUI() {
    debugWindow->setVisible(true);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;

    // Show the G-buffers
    debugPane->setCaption(GuiText("G-Buffers", GFont::fromFile(System::findDataFile("arial.fnt")), 16));
    debugPane->moveBy(2, 10);
    debugPane->beginRow();
//    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::WS_FACE_NORMAL));
    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::WS_NORMAL));
    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::WS_POSITION))->moveBy(40, 0);
    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::LAMBERTIAN))->moveBy(40, 0);
    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::GLOSSY))->moveBy(40, 0);
    debugPane->addTextureBox(gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL))->moveBy(40, 0);
    debugPane->endRow();
    debugPane->pack();
}


void App::onPose(Array<Surface::Ref>& surface, Array<Surface2D::Ref>& surface2D) {
    GApp::onPose(surface, surface2D);
    static float yaw = 150 * units::degrees();
    yaw += 2.0f * units::degrees();
    model->pose(surface, CFrame::fromXYZYPRDegrees(0,0,0,yaw,0,0));
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {

    // Generate the gbuffer
    gbuffer->prepare(rd, defaultCamera, 0, -1.0f / desiredFrameRate());
    Array<Surface::Ref> visibleArray;
    Surface::cull(defaultCamera, rd->viewport(), surface3D, visibleArray);
    Surface::renderIntoGBuffer(rd, visibleArray, gbuffer);

    // Make a pass over the screen, performing shading
    rd->push2D(); {
        shadingPass->args.set("wsNormal",   gbuffer->texture(GBuffer::Field::WS_NORMAL));
        shadingPass->args.set("wsPosition", gbuffer->texture(GBuffer::Field::WS_POSITION));
        shadingPass->args.set("lambertian", gbuffer->texture(GBuffer::Field::LAMBERTIAN));
        shadingPass->args.set("glossy",     gbuffer->texture(GBuffer::Field::GLOSSY));
        shadingPass->args.set("wsEye",      gbuffer->camera().coordinateFrame().translation);
 
        rd->setShader(shadingPass);

        Draw::fastRect2D(rd->viewport(), rd);
    } rd->pop2D();

    // Forward-render other objects
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
