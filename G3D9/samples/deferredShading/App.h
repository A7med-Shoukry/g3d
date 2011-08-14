#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class App : public GApp {
protected:

    ArticulatedModel2::Ref  model;
    GBuffer::Ref            gbuffer;
    Shader::Ref             shadingPass;
    CFrame                  previousCameraFrame;

    void makeGBuffer();
    void makeScene();
    void makeShader();
    void makeGUI();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onPose(Array<Surface::Ref>& surface, Array<Surface2D::Ref>& surface2D);
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);
};

#endif
