/**
 \file Viewer.h
 
 The base class for the more specialized Viewers
 
 \maintainer Morgan McGuire
 \author Eric Muller, Dan Fast, Katie Creel
 
 \created 2007-05-31
 \edited  2007-06-08
 */
#ifndef VIEWER_H
#define VIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

class Viewer {
public:
	virtual ~Viewer() {}
	virtual void onInit(const std::string& filename) = 0;
    virtual bool onEvent(const GEvent& e, App* app) { return false; }
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {}
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) = 0;
    virtual void onGraphics2D(RenderDevice* rd, App* app) {}

};

#endif 
