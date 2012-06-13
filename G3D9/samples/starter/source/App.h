/**
  @file App.h

  The G3D 9.0 default starter app is configured for OpenGL 3.0 and relatively recent
  GPUs.  To support older GPUs you may want to disable the framebuffer and film
  classes and use G3D::Sky to handle the skybox.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

#include "Scene.h"





class App : public GApp {

 









    Texture::Ref        m_texture;

    GuiDropDownList*    m_sceneDropDownList;
    Scene::Ref          m_scene;
    ShadowMap::Ref      m_shadowMap;

    /** Used for enabling dragging of objects with m_splineEditor.*/
    Entity::Ref         m_selectedEntity;

    /** Used for editing entity splines.*/
    PhysicsFrameSplineEditor::Ref   m_splineEditor;

    GuiDropDownList*    m_entityList;

    /** Don't allow object editing */
    bool                m_preventEntityDrag;
    bool                m_preventEntitySelect;

    bool                m_showAxes;
    bool                m_showLightSources;
    bool                m_showWireframe;


    /** Loads whatever scene is currently selected in the m_sceneDropDownList. */
    void loadScene();

    /** Save the current scene over the one on disk. */
    void saveScene();

    /** Called from onInit */
    void makeGUI();

    void selectEntity(const Entity::Ref& e);

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) override;

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& surface, Array<Surface2D::Ref>& surface2D) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) override;

    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual void onCleanup() override;

    /** Sets m_endProgram to true. */
    virtual void endProgram();
};

#endif
