/**
 \file App.cpp
 
 App that allows viewing of 2D and 3D assets
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2010-01-04
 */
#include "App.h"
#include "ArticulatedViewer.h"
#include "TextureViewer.h"
#include "FontViewer.h"
#include "BSPViewer.h"
#include "MD2Viewer.h"
#include "GUIViewer.h"
#include "EmptyViewer.h"
#include "VideoViewer.h"
#include "IconSetViewer.h"


App::App(const GApp::Settings& settings, const std::string& file) :
    GApp(settings),
    viewer(NULL),
    filename(file) {

    m_debugTextColor = Color3::black();
    m_debugTextOutlineColor = Color3::white();

    shadowMap = ShadowMap::create("Shadow Map", 2048);
    shadowMap->setPolygonOffset(2.0, 2.0);
    setDesiredFrameRate(60);

    catchCommonExceptions = true;
}


void App::onInit() {
    showRenderingStats = false;
    developerWindow->cameraControlWindow->setVisible(false);
    window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");

    lighting = Lighting::create();
    lighting->lightArray.clear();
    lighting->lightArray.append( GLight::directional(Vector3(1,1,1), Radiance3(8.5f)));
    lighting->lightArray.append( GLight::directional(Vector3(-1,0,-1), Radiance3(0.8f, 0.9f, 1), false));
    lighting->environmentMapConstant = 0.65f;
    lighting->environmentMapTexture = Texture::fromFile(System::findDataFile("noonclouds/noonclouds_*.png"), ImageFormat::AUTO(), Texture::DIM_CUBE_MAP_NPOT, Texture::Settings::cubeMap());
    defaultCamera.setFarPlaneZ(-1000);
    defaultCamera.setNearPlaneZ(-0.05f);

    // Don't clip to the near plane
    glDisable(GL_DEPTH_CLAMP);

	
    colorClear = Color3::white() * 0.9f;
    //modelController = ThirdPersonManipulator::create();

    setViewer(filename);
}


void App::onCleanup() {
    delete viewer;
    viewer = NULL;
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }

    switch (e.type) {
    case GEventType::FILE_DROP:
        {
            Array<std::string> fileArray;
            window()->getDroppedFilenames(fileArray);
            setViewer(fileArray[0]);
            return true;
        }
        
    default:;
    }
    
    if (viewer != NULL) {
        return viewer->onEvent(e, this);
    } else {
        return false;
    }
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Make the camera spin when the debug controller is not active
    if (false) {
        static float angle = 0;
        angle += rdt;
        float radius = 5.5;
        defaultCamera.setPosition(Vector3(cos(angle), 0, sin(angle)) * radius);
        defaultCamera.lookAt(Vector3(0,0,0));
    }

    // let viewer sim with time step if needed
    if (viewer != NULL) {
        viewer->onSimulation(rdt, sdt, idt);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& posed3D) {

    Lighting::Ref localLighting = lighting;
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(colorClear);
    rd->clear(true, true, true);

    rd->enableLighting();
        rd->setAmbientLightColor(Color3::white() * 0.3f);
            
        // Lights
        for (int L = 0; L < iMin(7, localLighting->lightArray.size()); ++L) {
            rd->setLight(L, localLighting->lightArray[L]);
        }

        // Render the file that is currently being viewed
        if (viewer != NULL) {
            viewer->onGraphics(rd, this, localLighting);
        }
        
        for (int i = 0; i < posed3D.size(); ++i) {
            posed3D[i]->render(rd);
        }
	rd->disableLighting();
}


void App::setViewer(const std::string& newFilename) {
    filename = newFilename;

    CFrame cframe(Vector3(0,8,15));
    cframe.lookAt(Point3::zero());
    defaultCamera.setCoordinateFrame(cframe);
    defaultController->setFrame(defaultCamera.coordinateFrame());

    //modelController->setFrame(CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180))));
    delete viewer;
    viewer = NULL;
    shadowMap->setSize(0);
    
    std::string ext = toLower(filenameExt(filename));
    std::string base = toLower(filenameBase(filename));
    
    if ((ext == "3ds") ||
        (ext == "ifs") ||
        (ext == "obj") ||
        (ext == "ply2") ||
        (ext == "off") ||
        (ext == "ply") ||
        (ext == "any" && endsWith(base, ".am"))) {
        
        shadowMap->setSize(2048);
        
        viewer = new ArticulatedViewer();
        
    } else if (Texture::isSupportedImage(filename)) {
        
        // Images can be either a Texture or a Sky, TextureViewer will figure it out
        viewer = new TextureViewer();

		// Angle the camera slightly so a sky/cube map doesn't see only 1 face
		defaultController->setFrame(Matrix3::fromAxisAngle(Vector3::unitY(), halfPi() / 2.0) * Matrix3::fromAxisAngle(Vector3::unitX(), halfPi() / 2.0));

    } else if (ext == "fnt") {
        
        viewer = new FontViewer(debugFont);
        
    } else if (ext == "bsp") {
        
        viewer = new BSPViewer();
        
    } else if (ext == "md2") {
        
        viewer = new MD2Viewer();
        
    } else if (ext == "gtm") {
        
        viewer = new GUIViewer(this);

    } else if (ext == "icn") {
        
        viewer = new IconSetViewer(debugFont);
        
    } else if (ext == "pk3") {
        // Something in Quake format - figure out what we should load
        Array <std::string> files;
        bool set = false;
        
        // First, try for a .bsp map
        std::string search = filename + "/maps/*";
        FileSystem::getFiles(search, files, true);

        for (int t = 0; t < files.length(); ++t) {
            
            if (filenameExt(files[t]) == "bsp") {
                
                filename = files[t];
                viewer = new BSPViewer();
                set = true;
            }
        }
        if (!set) {
            viewer = new EmptyViewer();
        }

    } else if (ext == "avi" || ext == "wmv" || ext == "mp4" || ext == "asf" || 
               (ext == "mov") || (ext == "dv") || (ext == "qt") || (ext == "asf") ||
               (ext == "mpg")) {
        viewer = new VideoViewer();

    } else {
        
        viewer = new EmptyViewer();
	
    }

    if (viewer != NULL) {
        viewer->onInit(filename);
    }
    
    window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");
}
