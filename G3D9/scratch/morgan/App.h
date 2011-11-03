/**
  @file App.h

  The G3D 9.0 default starter app is configured for OpenGL 3.0 and relatively recent
  GPUs.  To support older GPUs you may want to disable the framebuffer and film
  classes and use G3D::Sky to handle the skybox.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class App : public GApp {
public:
    
    Texture::Ref            texture;
    ArticulatedModel2::Ref  model;
    Lighting::Ref           lighting;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);

    virtual bool onEvent(const GEvent& e);
};

#endif
