/** \file App.cpp */
#include "App.h"
#include <typeinfo>

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

/** 
 CubeMap<Image3>::Ref cube = CubeMap<Image3>::create(...);
*/
template<class Image>
class CubeMap : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer< class CubeMap<Image> > Ref;

protected:

    int                         m_width;
    Array<typename Image::Ref>  m_imageArray;

    CubeMap() {}

    void init(const Array<typename Image::Ref>& imageArray) {
        m_imageArray = imageArray;
        debugAssert(imageArray.size() == 6);
        m_width = m_imageArray[0]->width();
        for (int i = 0; i < 6; ++i) {
            debugAssert(m_imageArray[i]->width()  == m_width);
            debugAssert(m_imageArray[i]->height() == m_width);
        }
    }

public:

    static Ref create(const std::string& fileSpec) {
        // TODO
        return NULL;
    }

    /** Retains references to the underlying images. */
    static Ref create(const Array<typename Image::Ref>& imageArray) {
        Ref c = new CubeMap<Image>();
        c->init(imageArray);
        return c;
    }

    typename Image::ComputeType bilinear(const Vector3& v) const;

    typename Image::ComputeType nearest(const Vector3& v) const;

    /** Return the image representing one face. */
    const typename Image::Ref face(CubeFace f) const {
        return m_imageArray[f];
    }
    
    /** Returns the width of one side, which must be the same as the height. */
    int width() const {
        return m_width;
    }

    /** Returns the height of one side in pixels, which must be the same as the height. */
    int height() const {
        // Width and height are the same in this class.
        return m_width;
    }

};


/**
 \brief Separates a large array into subarrays by their typeid().

 Example:
 \code
 Array<Surface::Ref> all = ...;
 Array< Array<Surface::Ref> > derivedArray;
 categorizeByDerivedType<Surface::Ref>(all, derivedArray);
 \endcode
 */
template<class PointerType>
void categorizeByDerivedType(const Array<PointerType>& all, Array< Array<PointerType> >& derivedArray) {
    derivedArray.fastClear();

    // Allocate space for the worst case, so that we don't have to copy arrays
    // all over the place during resizing.
    derivedArray.reserve(all.size());

    Table<std::type_info *const, int> typeInfoToIndex;
    // Allocate the table elements in a memory area that can be cleared all at once
    // without invoking destructors.
    typeInfoToIndex.clearAndSetMemoryManager(AreaMemoryManager::create(100 * 1024));

    for (int s = 0; s < all.size(); ++s) {
        const PointerType& instance = all[s];
        
        bool created = false;
        int& index = typeInfoToIndex.getCreate(const_cast<std::type_info*const>(&typeid(*instance)), created);
        if (created) {
            // This is the first time that we've encountered this subclass.
            // Allocate the next element of subclassArray to hold it.
            index = derivedArray.size();
            derivedArray.next();
        }
        derivedArray[index].append(instance);
    }
}


int main(int argc, char** argv) {
    /*
    Array<Surface::Ref> all;
    Array< Array<Surface::Ref> > derivedArray;
    categorizeByDerivedType<Surface::Ref>(all, derivedArray);
    */


#if 0
    Image1::Ref im = Image1::createEmpty(32, 64);
    for (int y = 0; y < im->height(); ++y) {
        for (int x = 0; x < im->width(); ++x) {
            im->set(x, y, Color1(x / float(im->width() - 1)));
        }
    }
    im->save("test.png", GImage::PNG16);
    debugPrintf("Done\n");
    return 0;
#endif

    (void)argc; (void)argv;
    GApp::Settings settings;
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1100; 
    settings.window.height      = 1100;
    settings.window.defaultIconFilename = "32x32.png";


#   ifdef G3D_WIN32
	if (FileSystem::exists("data-files", false)) {
            // Running on Windows, building inside the starter directory
            chdir("data-files");
        } else if (FileSystem::exists("../samples/starter/data-files", false)) {
            // Running on Windows, building from the G3D.sln project (TODO: remove this from your program!)
            chdir("../samples/starter/data-files");
        }
#   endif
    debugPrintf("Running in %s\n", FileSystem::currentDirectory().c_str());
    //CubeMap<Image3>::Ref im = CubeMap<Image3>::create(System::findDataFile("test/testcube_*.jpg"));
    
    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
      
    Random r;
}

class ImageLoader : public GThread {
public:
    typedef ReferenceCountedPointer<ImageLoader> Ref;

    const std::string filename;
    GLuint  pbo;
    GLvoid* ptr;
    GImage  im;
    int     sidePixels;

    /** Must be invoked on the OpenGL thread */
    ImageLoader(const std::string& filename, int sidePixels) : GThread(filename), filename(filename), sidePixels(sidePixels) {
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        size_t size = sidePixels * sidePixels * 3;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, size, NULL, GL_STREAM_DRAW);
        ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
    }

    virtual void threadMain() {
        im.load(filename);
        debugAssert(im.width() == sidePixels);
        debugAssert(im.height() == sidePixels);
        debugAssert(im.channels() == 3);
        System::memcpy(ptr, im.byte(), im.width() * im.height() * im.channels());
    }

    /** Call on the GL thread after threadMain is done */
    void unmap() {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        ptr = NULL;
    }

    /** Must be invoked on the OpenGL thread */
    virtual ~ImageLoader() {
        glDeleteBuffers(1, &pbo);
        pbo = GL_NONE;
    }
};

Texture::Ref sky;
Texture::Ref tex;

void loadt2D() {    
    Stopwatch stopwatch;

    // Load an image from disk.  It is 2048^2 x 3
    GImage image("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01bk.png");
    stopwatch.after("Load from disk");

    // Create an empty texture in bilinear-nearest-nearest mode, with MIP-map generation disabled
    tex = Texture::createEmpty("tex", image.width(), image.height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    stopwatch.after("Create GL texture");

    // Create a pbo
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    stopwatch.after("Create GL PBO");

    // Allocate memory on the GPU
    const size_t size = image.width() * image.height() * 3;
    glBufferData(GL_PIXEL_UNPACK_BUFFER, size, NULL, GL_STREAM_DRAW);
    stopwatch.after("Allocate PBO space");

    // Map the memory to the CPU
    GLvoid* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    stopwatch.after("Map PBO");

    // Copy data to the mapped memory (using SIMD wide loads and stores)
    System::memcpy(ptr, image.byte(), size);
    stopwatch.after("Memcpy");

    // Unmap the memory
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    ptr = NULL;
    stopwatch.after("Unmap PBO");
    // Even if we sleep after unmapping the PBO, the following glTexImage2D still takes 0.14 s
    // System::sleep(1);
    // stopwatch.after("sleep");

    // Copy the PBO to the texture
    glBindTexture(tex->openGLTextureTarget(), tex->openGLID());
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(), GL_RGB, GL_BYTE, (GLvoid*)0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.width(), image.height(), 0, GL_RGB, GL_BYTE, (GLvoid*)0);
    glBindTexture(tex->openGLTextureTarget(), GL_NONE);
    stopwatch.after("glTexImage2D");

    // Unbind PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
    stopwatch.after("Unbind PBO");

    // Delete the PBO
    glDeleteBuffers(1, &pbo);
    pbo = GL_NONE;
    stopwatch.after("Delete PBO");
}


void App::onInit() {
//    GuiWindow::Ref g = GuiWindow::create();

    Framebuffer::Ref fb = Framebuffer::create("Framebuffer");
    Texture::Ref rg16f = Texture::createEmpty("rgb16f", 640, 400, ImageFormat::RG16F(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
    Texture::Ref rgb8 = Texture::createEmpty("rgb8", 640, 400, ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());


    fb->set(Framebuffer::COLOR0, rg16f);
    fb->set(Framebuffer::COLOR1, rgb8);

    renderDevice->push2D(fb);
    Draw::rect2D(Rect2D::xywh(10, 20, 100, 100), renderDevice);
    renderDevice->pop2D();

    ::exit(0);

    GImage im(10,10,3);
    show(im);

    Stopwatch stopwatch;


//    loadt2D();

    stopwatch.tick();

    Texture::Settings settings = Texture::Settings::cubeMap();
    settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
    sky = Texture::createEmpty("cubemap", 0, 0, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, settings);


    // Using multiple threads cuts the load time from disk from 0.9s to 0.6s.
    // Using PBO and multiple threads for DMA has no measurable impact on performance over single-threaded glTexImage2D from
    // the CPU memory--both take about 1.5s.
    {
        ThreadSet threadSet;
        int w = 2048;
        Array<ImageLoader::Ref> image;
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01bk.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01dn.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01lf.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01ft.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01rt.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01up.png", w));
        for (int i = 0; i < 6; ++i) {
            threadSet.insert(image[i]);
        }

        // Run the threads, reusing the current thread and blocking until
        // all complete
        threadSet.start();

        // On the main thread, resize the textures while uploading happens on the other threads
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        glBindTexture(sky->openGLTextureTarget(), sky->openGLID());
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, GL_RGB8, w, w, 0, GL_RGB, GL_BYTE, (GLvoid*)0);
        }
        glBindTexture(sky->openGLTextureTarget(), GL_NONE);

        threadSet.waitForCompletion();

        for (int i = 0; i < 6; ++i) {
            image[i]->unmap();
        }

        // Copy PBOs to GL texture faces
        glBindTexture(sky->openGLTextureTarget(), sky->openGLID());
        for (int i = 0; i < 6; ++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, image[i]->pbo);
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, 0, 0, w, w, GL_RGB, GL_BYTE, (GLvoid*)0);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        glBindTexture(sky->openGLTextureTarget(), GL_NONE);
    }


    /*
    Texture::Settings settings  = Texture::Settings::cubeMap();
    settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
    Texture::Ref t = Texture::fromFile("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01*.png", ImageFormat::AUTO(), Texture::DIM_CUBE_MAP, settings, skyprocess);
    */
    stopwatch.tock();
    debugPrintf("PBO thread: %fs\n", stopwatch.elapsedTime());
    /*
    stopwatch.tick();
    sky = Texture::fromFile("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01*.png", ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, settings);
    stopwatch.tock();
    debugPrintf("Texture: %fs\n", stopwatch.elapsedTime());
    */
//    ::exit(0);

    // Called before the application loop beings.  Load data here and
    // not in the constructor so that common exceptions will be
    // automatically caught.
    
    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);
    showRenderingStats = true;


    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    // Example of using a callback; you can also listen for events in onEvent or bind controls to data
    m_sceneDropDownList = debugPane->addDropDownList("Scene", Scene::sceneNames(), NULL, GuiControl::Callback(this, &App::loadScene));
    debugPane->addButton(GuiText("q", GFont::fromFile(System::findDataFile("icon.fnt")), 14), this, &App::loadScene, GuiTheme::TOOL_BUTTON_STYLE)->moveRightOf(m_sceneDropDownList);
    debugPane->addLabel("Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal.")->setSize(Vector2(180, 200));
    
    debugWindow->pack();
    debugWindow->moveTo(Vector2(0, window()->height() - debugWindow->rect().height()));

    // Start wherever the developer HUD last marked as "Home"
    defaultCamera.setCoordinateFrame(bookmark("Home"));

    renderDevice->setColorClearValue(Color3::white());
    m_shadowMap = ShadowMap::create();
    m_font = GFont::fromFile(System::findDataFile("arial.fnt"));

//    loadScene();
    
    //show(Image3::createEmpty(600, 300));
    //show(Image3::createEmpty(600, 300));
}


void App::loadScene() {
    const std::string& sceneName = m_sceneDropDownList->selectedValue().text();

    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    // Load the scene
    m_scene = Scene::create(sceneName, defaultCamera);
}


void App::onAI() {
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt; (void)sdt; (void)rdt;
    // Add physical simulation here.  You can make your time
    // advancement based on any of the three arguments.
//    m_scene->onSimulation(sdt);
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }
    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((e.type == GEventType::GUI_ACTION) && (e.gui.control == m_button)) { ... return true;}
    // if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::TAB)) { ... return true; }

    return false;
}


void App::onUserInput(UserInput* ui) {
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D) {
    // Append any models to the arrays that you want to later be rendered by onGraphics()
//    m_scene->onPose(surfaceArray);
    (void)surface2D;
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    Draw::skyBox(rd, sky, 1.0, 1.0);
/*    if (m_scene->lighting()->environmentMapTexture.notNull()) {
        Draw::skyBox(rd, m_scene->lighting()->environmentMapTexture, m_scene->lighting()->environmentMapConstant);
    }

    // Render all objects (or, you can call Surface methods on the
    // elements of posed3D directly to customize rendering.  Pass a
    // ShadowMap as the final argument to create shadows.)
    Surface::sortAndRender(rd, defaultCamera, surface3D, m_scene->lighting(), m_shadowMap);
    */

    Draw::axes(rd);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
    

    
    /*
    m_shader->args.set("cubemap", m_scene->lighting()->environmentMapTexture);
    rd->setShader(m_shader);
    Array<Surface::Ref> a;
    m_sphere->pose(a);
    a[0]->sendGeometry(rd);
    */

}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction

    rd->setTexture(0, tex);
    Draw::rect2D(Rect2D::xywh(0,0,400,400), rd);

    Draw::rect2DBorder(Rect2D::xywh(100,100,200,100), rd);
    m_font->draw2DWordWrap(rd, 200, "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal.", Point2(100,100), 12);
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}


void App::endProgram() {
    m_endProgram = true;
}
