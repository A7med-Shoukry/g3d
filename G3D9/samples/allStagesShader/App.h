/**
  @file App.h

  This sample is taken from http://prideout.net/blog/?p=48, and adapted to work with G3D Shader2.
  It uses all five currently programmable stages of the shader pipline: vertex, tesselation control,
  tesselation evaluation, geometry, and fragment.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

class App : public GApp {

    Shader2::Ref            m_allStagesShader;
    Array<Surface::Ref>     m_sceneGeometry;
    float                   m_innerTessLevel;
    float                   m_outerTessLevel;
public:
    
    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array<SurfaceRef>& surface);
    virtual bool onEvent(const GEvent& e) override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    void reloadShaders();
    void makeGUI();
};

#endif
