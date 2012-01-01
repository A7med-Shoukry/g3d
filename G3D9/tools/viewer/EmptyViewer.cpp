/**
 \file EmptyViewer.cpp
 
 If the person tried to load a file that wasn't valid, this default viewer is displayed
 
 \maintainer Morgan McGuire
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2011-12-31
 */
#include "EmptyViewer.h"

EmptyViewer::EmptyViewer(){}


void EmptyViewer::onInit(const std::string& filename) {}


void EmptyViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    screenPrintf("Viewer");
    screenPrintf(System::version().c_str());
    screenPrintf("http://g3d.sf.net");
    screenPrintf("\n");
    screenPrintf("\n");
	screenPrintf("Drag and drop an file to view.\n");
	screenPrintf("\n");
	screenPrintf("Image Formats: png, jpg, bmp, tga, pcx, dds, ppm, ico, gif");
	screenPrintf("3D Formats:    obj, 3ds, pk3, md2, md3, bsp, off, ply, ply2, ifs, am.any");
    screenPrintf("GUI Formats:   fnt, gtm");
#   ifndef NO_FFMPEG
    screenPrintf("Video Formats: mp4, avi, mp4, mov, ogg, asf, wmv");
#   endif

}
