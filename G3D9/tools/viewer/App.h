/**
 @file App.h
 
 App that allows viewing of 3D assets
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2010-02-26
 */
#ifndef APP_H
#define APP_H

#include "G3D/G3DAll.h"
#include "GLG3D/GLG3D.h"
class Viewer;

class App : public GApp {
private:
    Lighting::Ref	   lighting;
    Viewer*	           viewer;
    std::string	       filename;
    
public:
    /** Used by GUIViewer */
    Color4						colorClear;
    
    /** Used by ArticulatedViewer */
    ShadowMap::Ref              shadowMap;
    
    App(const GApp::Settings& settings = GApp::Settings(), const std::string& file = "");
    
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surfaceArray) override;
    virtual void onGraphics2D(RenderDevice *rd, Array< Surface2D::Ref > &surface2D) override;
    virtual void onCleanup() override;
    virtual bool onEvent(const GEvent& event) override;
    
private:
    /** Called from onInit() and after a FILE_DROP in onEvent()*/
    void setViewer(const std::string& newFilename);
};

#endif
