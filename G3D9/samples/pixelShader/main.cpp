/**
  \file shader/main.cpp

  Example of using G3D shaders and GUIs.
  
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class App : public GApp {
private:
    Lighting::Ref       lighting;
    SkyRef              sky;
    IFSModel::Ref       model;

    Shader::Ref         phongShader;
    
    float               diffuseScalar;
    int                 diffuseColorIndex;

    float               specularScalar;
    int                 specularColorIndex;

    float               reflect;
    float               shine;

    ////////////////////////////////////
    // GUI

    /** For dragging the model */
    ThirdPersonManipulatorRef manipulator;
    Array<GuiText>      colorList;

    void makeGui();
    void makeColorList();
    void makeLighting();
    void configureShaderArgs(const Lighting::Ref localLighting);

public:

    App() : diffuseScalar(0.6f), specularScalar(0.5f), reflect(0.1f), shine(20.0f) {}

    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D);
};


void App::onInit() {
    window()->setCaption("Pixel Shader Demo");
        
    phongShader = Shader::fromFiles("phong.vrt", "phong.pix");
    model = IFSModel::fromFile(System::findDataFile("teapot.ifs"));

    makeLighting();
    makeColorList();
    makeGui();

    // Color 1 is red
    diffuseColorIndex = 1;
    // Last color is white
    specularColorIndex = colorList.size() - 1;
    
    defaultCamera.setPosition(Vector3(1.0f, 1.0f, 2.5f));
    defaultCamera.setFieldOfView(45 * units::degrees(), GCamera::VERTICAL);
    defaultCamera.lookAt(Point3::zero());

    // Add axes for dragging and turning the model
    manipulator = ThirdPersonManipulator::create();
    addWidget(manipulator);

    // Turn off the default first-person camera controller and developer UI
    defaultController->setActive(false);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    rd->setProjectionAndCameraMatrix(defaultCamera);

    Draw::skyBox(rd, lighting->environmentMapTexture, lighting->environmentMapConstant);

    rd->pushState(); {
        // Pose our model based on the manipulator axes
        Surface::Ref posedModel = model->pose(manipulator->frame());
        
        // Enable the shader
        configureShaderArgs(lighting);
        rd->setShader(phongShader);

        // Send model geometry to the graphics card
        rd->setObjectToWorldMatrix(posedModel->coordinateFrame());
        posedModel->sendGeometry(rd);
    } rd->popState();

    // Render other objects, e.g., the 3D widgets
    Surface::sortAndRender(rd, defaultCamera, surface3D, lighting);   
}


void App::configureShaderArgs(const LightingRef lighting) {
    const GLight&       light         = lighting->lightArray[0];
    const Color3&       diffuseColor  = colorList[diffuseColorIndex].element(0).color(Color3::white()).rgb();
    const Color3&       specularColor = colorList[specularColorIndex].element(0).color(Color3::white()).rgb();
    Shader::ArgList&    args          = phongShader->args;
    
    // Viewer
    args.set("wsEyePosition",       defaultCamera.coordinateFrame().translation);
    
    // Lighting
    args.set("wsLight",             light.position.xyz().direction());
    args.set("lightColor",          light.color);
    args.set("ambient",             Color3(0.3f));
    args.set("environmentMap",      lighting->environmentMapTexture);
    args.set("environmentConstant", lighting->environmentMapConstant);
    
    // Material
    args.set("diffuseColor",        diffuseColor);
    args.set("diffuseScalar",       diffuseScalar);
    
    args.set("specularColor",       specularColor);
    args.set("specularScalar",      specularScalar);
    
    args.set("shine",               shine);
    args.set("reflect",             reflect);
}


void App::makeColorList() {
    GFont::Ref iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));

    // Characters in icon font that make a solid block of color
    static const char* block = "gggggg";

    float size = 18;
    int N = 10;
    colorList.append(GuiText(block, iconFont, size, Color3::black(), Color4::clear()));
    for (int i = 0; i < N; ++i) {
        colorList.append(GuiText(block, iconFont, size, Color3::rainbowColorMap((float)i / N), Color4::clear()));
    }
    colorList.append(GuiText(block, iconFont, size, Color3::white(), Color4::clear()));
}


void App::makeGui() {
    GuiWindow::Ref gui = GuiWindow::create("Material Parameters");    
    GuiPane* pane = gui->pane();

    pane->beginRow();
    pane->addSlider("Diffuse", &diffuseScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &diffuseColorIndex)->setWidth(80);
    pane->endRow();

    pane->beginRow();
    pane->addSlider("Specular", &specularScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &specularColorIndex)->setWidth(80);
    pane->endRow();
    
    pane->addSlider("Reflectivity", &reflect, 0.0f, 1.0f);
    pane->addSlider("Shininess", &shine, 1.0f, 100.0f);
    
    gui->pack();
    addWidget(gui);
    gui->moveTo(Point2(10, 10));
}


void App::makeLighting() {
    Lighting::Specification spec;
    spec.lightArray.append(GLight::directional(Vector3(1.0f, 1.0f, 1.0f), Color3(1.0f), false));

    // The environmentMap is a cube of six images that represents the incoming light to the scene from
    // the surrounding environment.  G3D specifies all six faces at once using a wildcard and loads
    // them into an OpenGL cube map.
    spec.environmentMapConstant = 1.0f;
    spec.environmentMapTexture.filename   = FilePath::concat(System::findDataFile("noonclouds"), "noonclouds_*.png");
    spec.environmentMapTexture.dimension  = Texture::DIM_CUBE_MAP;
    spec.environmentMapTexture.settings   = Texture::Settings::cubeMap();
    spec.environmentMapTexture.preprocess = Texture::Preprocess::gamma(2.1f);
    // Reduce memory size required to work on older GPUs
    spec.environmentMapTexture.preprocess.scaleFactor = 0.25f;
    spec.environmentMapTexture.settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
    
    lighting = Lighting::create(spec);
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {

#   ifdef G3D_WIN32
      if (! FileSystem::exists("phong.pix", false) && FileSystem::exists("G3D.sln", false)) {
          // The program was started from within Visual Studio and is
          // running with cwd = G3D/VC10/.  Change to
          // the appropriate sample directory.
          chdir("../samples/pixelShader/data-files");
      } else if (FileSystem::exists("data-files")) {
          chdir("data-files");
      }
#   endif

    return App().run();
}
