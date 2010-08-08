/**
  @file App.h

  The G3D 8.0 default starter app is configured for OpenGL 3.0 and relatively recent
  GPUs.  To support older GPUs you may want to disable the framebuffer and film
  classes and use G3D::Sky to handle the skybox.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#include "Scene.h"

class App : public GApp {
    ShadowMap::Ref      m_shadowMap;
    
    Lighting::Ref                m_lighting;
    ArticulatedModel::Ref        m_model;
    
public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& surface, Array<Surface2D::Ref>& surface2D);

    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);

};

#endif
