/**
 \file GLG3D/GLG3D.h

 This header includes all of the GLG3D libraries in 
 appropriate namespaces.

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-08-07
 \edited  2011-07-22

 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_GLG3D_h
#define G3D_GLG3D_h

#include "G3D/G3D.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/Texture.h"
#include "GLG3D/glFormat.h"
#include "GLG3D/Milestone.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/GFont.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/Draw.h"
#include "GLG3D/tesselate.h"
#include "GLG3D/GApp.h"
#include "GLG3D/Surface.h"
#include "GLG3D/IFSModel.h"
#include "GLG3D/MD2Model.h"
#include "GLG3D/MD3Model.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/SDLWindow.h"
#include "GLG3D/Shader.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Shape.h"
#include "GLG3D/Renderbuffer.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Widget.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/BSPMAP.h"
#include "GLG3D/GKey.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/Material.h"
#include "GLG3D/SuperShader.h"
#include "GLG3D/GaussianBlur.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/DirectionHistogram.h"
#include "GLG3D/SuperBSDF.h"
#include "GLG3D/Component.h"
#include "GLG3D/Film.h"
#include "GLG3D/Tri.h"
#include "GLG3D/TriTree.h"
#include "GLG3D/Profiler.h"
#include "GLG3D/SurfaceElement.h"

#include "GLG3D/GuiTheme.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiMenu.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiNumberBox.h"
#include "GLG3D/GuiFunctionBox.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/IconSet.h"

#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/DeveloperWindow.h"
#include "GLG3D/VideoRecordDialog.h"

#include "GLG3D/VideoInput.h"
#include "GLG3D/VideoOutput.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/GBuffer.h"

#include "GLG3D/Discovery.h"
#include "GLG3D/GEntity.h"
#include "GLG3D/ArticulatedModel2.h"
#include "GLG3D/CPUVertexArray.h"

#include "GLG3D/PhysicsFrameSplineEditor.h"

#ifdef G3D_OSX
#include "GLG3D/CarbonWindow.h"
#endif

#ifdef G3D_WIN32
#include "GLG3D/Win32Window.h"
#include "GLG3D/DXCaps.h"
#endif

#ifdef G3D_LINUX
#include "GLG3D/X11Window.h"
#endif

#ifdef G3D_OSX
#include "GLG3D/SDLWindow.h"
#endif


// Set up the linker on Windows
#ifdef _MSC_VER

#   pragma comment(lib, "ole32")
#   pragma comment(lib, "opengl32")
#   pragma comment(lib, "glu32")
#   pragma comment(lib, "shell32") // for drag drop

/** \def G3D_STATIC_LINK_FFMPEG If you #define this before including GLG3D.h or G3DAll.h,
 then G3D will statically link to FFMPEG on Windows and you do not need to distribute its DLLs.  By
 default G3D dynamically links to FFMPEG for licensing reasons, which means you need to 
 distribute the bin/ *.dll files with your program.
*/
//#   define G3D_STATIC_LINK_FFMPEG


#ifndef NO_FFMPEG
#   ifndef G3D_STATIC_LINK_FFMPEG
//      DLL version
#       ifdef G3D_64BIT
#           pragma comment(lib, "avutil-50-64")
#           pragma comment(lib, "avcodec-52-64")
#           pragma comment(lib, "avformat-52-64")
#           pragma comment(lib, "swscale-0-64")
#       else
#           pragma comment(lib, "avutil-50")
#           pragma comment(lib, "avcodec-52")
#           pragma comment(lib, "avformat-52")
#           pragma comment(lib, "swscale-0")
#       endif
#   else
//      LIB-only version
#       ifdef G3D_64BIT
#           pragma comment(lib, "mingwrt-64")
#           pragma comment(lib, "avutil-64")
#           pragma comment(lib, "avcodec-64")
#           pragma comment(lib, "avformat-64")
#           pragma comment(lib, "swscale-64")
#        else
#           pragma comment(lib, "mingwrt")
#           pragma comment(lib, "avutil")
#           pragma comment(lib, "avcodec")
#           pragma comment(lib, "avformat")
#           pragma comment(lib, "swscale")
#        endif
#   endif
#endif // NO_FFMPEG

#   ifdef _DEBUG
#       ifdef G3D_64BIT
#	        pragma comment(lib, "GLG3D-64d")
#       else
#	        pragma comment(lib, "GLG3Dd")
#       endif
#   else
#       ifdef G3D_64BIT
#           pragma comment(lib, "GLG3D-64")
#       else
#           pragma comment(lib, "GLG3D")
#       endif
#   endif

#endif
#endif
