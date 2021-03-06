/**
   \file GApp.h
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2003-11-03
   \edited  2011-05-12
*/

#ifndef G3D_GApp_h
#define G3D_GApp_h

#include "G3D/Stopwatch.h"
#include "GLG3D/GFont.h"
#include "G3D/GCamera.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/DeveloperWindow.h"
#include "G3D/GThread.h"
#include "GLG3D/Shape.h"
#include "GLG3D/Film.h"
#include "GLG3D/Shape.h"
#include "GLG3D/Renderbuffer.h"
#include "GLG3D/DepthOfField.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class UserInput;
class Log;

/** Used with debugDraw. */
typedef int DebugID;

/**
 \brief Schedule a G3D::Shape for later rendering.

 Adds this shape and the specified information to the current G3D::GApp::debugShapeArray, 
 to be rendered at runtime for debugging purposes.

 Sample usage is:
 \code
 debugDraw(new SphereShape(Sphere(center, radius)));
 \endcode

 \beta

 \param displayTime Real-world time in seconds to display the shape
 for.  A shape always displays for at least one frame. 0 = one frame.
 inf() = until explicitly removed by the GApp.

 \return The ID of the shape, which can be used to clear it for shapes that are displayed "infinitely".

 \sa debugPrintf, logPrintf, screenPrintf, GApp::drawDebugShapes, GApp::removeDebugShape, GApp::removeAllDebugShapes
 */
DebugID debugDraw
(const Shape::Ref& shape, 
 float             displayTime = 0.0f,
 const Color4&     solidColor  = Color3::white(), 
 const Color4&     wireColor   = Color3::black(), 
 const CFrame&     cframe      = CFrame());

/**
 \brief Optional base class for quickly creating 3D applications.

 GApp has several event handlers implemented as virtual methods.  It invokes these in
 a cooperative, round-robin fashion.  This avoids the need for threads in most
 applications.  The methods are, in order of invocation from GApp::oneFrame:
 
 <ul>
 <li> GApp::onEvent - invoked once for each G3D::GEvent
 <li> GApp::onUserInput - process the current state of the keyboard, mouse, and game pads
 <li> GApp::onNetwork - receive network packets; network <i>send</i> occurs wherever it is needed
 <li> GApp::onAI - game logic and NPC AI
 <li> GApp::onSimulation - physical simulation
 <li> GApp::onPose - create an array of Surface and Surface2D for rendering
 <li> GApp::onWait - tasks to process while waiting for the next frame to start
 <li> GApp::onGraphics - render the Surface and Surface2D arrays.  By default, this invokes two helper methods:
   <ul>
    <li> GApp::onGraphics3D - render the Surface array and any immediate mode 3D 
    <li> GApp::onGraphics2D - render the Surface2D array and any immediate mode 2D 
   </ul>
 </ul>
 
 The GApp::run method starts the main loop.  It invokes GApp::onInit, runs the main loop
 until completion, and then invokes GApp::onCleanup.

 onWait runs before onGraphics because the beginning of onGraphics causes the CPU to block, waiting for the GPU
 to complete the previous frame.

 When you override a method, invoke the GApp version of that method to ensure that Widget%s still work
 properly.  This allows you to control whether your per-app operations occur before or after the Widget ones.

 \sa GApp::Settings, OSWindow, RenderDevice, G3D_START_AT_MAIN
*/
class GApp {
public:
    friend class OSWindow;
    friend class VideoRecordDialog;

    class Settings {
    public:
        OSWindow::Settings       window;

        /**
           If "<AUTO>", will be set to the directory in which the executable resides.
           This is used to invoke System::setDataDir()
        */
        std::string             dataDir;
        
        /**
           Can be relative to the G3D data directory (e.g. "font/dominant.fnt")
           or relative to the current directory.
           Default is "console-small.fnt"
        */
        std::string             debugFontName;
        
        std::string             logFilename;

        /** If true, the G3D::DeveleloperWindow and G3D::CameraControlWindow will be enabled and
            accessible by pushing F12.
            These require osx.gtm, arial.fnt, greek.fnt, and icon.fnt to be in locations where
            System::findDataFile can locate them (the program working directory is one such location).
        */  
        bool                    useDeveloperTools;
        
        /** 
            When true, GAapp ensures that g3d-license.txt exists in the current
            directory.  That file is written from the return value of G3D::license() */
        bool                    writeLicenseFile;

        class FilmSettings {
        public:
            /** If true, allocate GApp::m_frameBuffer and use the m_film class when rendering.  
                On older GPUs the Film class may add too much memory or processing overhead.

                GApp::m_useFilm can be toggled to selectively disable use of Film outside of
                the GApp::onGraphics method (it is too late inside GApp::onGraphics).

                Defaults to true.*/
            bool                        enabled;

            /** Size of the film backbuffer. Set to -1, -1 to automatically size to the window.*/
            Vector2int16                dimensions;

            /** Formats to attempt to use for the Film, in order of decreasing preference */
            Array<const ImageFormat*>   preferredColorFormats;

            /** Formats to attempt to use for the Film, in order of decreasing preference. 
               NULL (or an empty list) indicates that no depth buffer should be allocated. 
               
               If you want separate depth and stencil attachments, you must explicitly allocate
               the stencil buffer yourself and attach it to the depth buffer.
              */
            Array<const ImageFormat*>   preferredDepthFormats;

            FilmSettings() : enabled(true), dimensions(-1, -1) {     

                preferredColorFormats.append(ImageFormat::R11G11B10F(), ImageFormat::RGB16F(), ImageFormat::RGBA8());
                preferredDepthFormats.append(ImageFormat::DEPTH24(), ImageFormat::DEPTH16(), ImageFormat::DEPTH32());
            }
        };

        FilmSettings            film;

        /** Arguments to the program, from argv.  The first is the name of the program. */
        Array<std::string>      argArray;
        
        Settings() : 
            dataDir("<AUTO>"), debugFontName("console-small.fnt"), 
            logFilename("log.txt"), useDeveloperTools(true), writeLicenseFile(true) {
        }

        Settings(int argc, const char* argv[]) : 
            dataDir("<AUTO>"), debugFontName("console-small.fnt"), 
            logFilename("log.txt"), useDeveloperTools(true), writeLicenseFile(true) {
            argArray.resize(argc);
            for (int i = 0; i < argc; ++i) {
                argArray[i] = argv[i];
            }
        }
    };

    class DebugShape {
    public:
        ShapeRef        shape;
        Color4          solidColor;
        Color4          wireColor;
        CFrame          frame;
        DebugID         id;
        /** Clear after this time (always draw before clearing) */
        RealTime        endTime;
    };

    /** Last DebugShape::id issued */
    DebugID              m_lastDebugID;


    /** \brief Shapes to be rendered each frame.  

        Added to by G3D::debugDraw.
        Rendered by drawDebugShapes();
        Automatically cleared once per frame.
      */
    Array<DebugShape>    debugShapeArray;

    /** \brief Draw everything in debugShapeArray.

        Subclasses should call from onGraphics3D() or onGraphics().
        This will sort the debugShapeArray from back to front
        according to the current camera.

        \sa debugDraw, Shape, DebugID, removeAllDebugShapes, removeDebugShape
     */
    void drawDebugShapes();

    /** 
        \brief Clears all debug shapes, regardless of their pending display time.

        \sa debugDraw, Shape, DebugID, removeDebugShape, drawDebugShapes
    */
    void removeAllDebugShapes();

    /** 
        \brief Clears just this debug shape (if it exists), regardless of its pending display time.

        \sa debugDraw, Shape, DebugID, removeAllDebugShapes, drawDebugShapes
    */
    void removeDebugShape(DebugID id);

private:

    /** Called from init. */
    void loadFont(const std::string& fontName);

    /** When recording, this dialog registers here */
    VideoRecordDialog*       m_activeVideoRecordDialog;

    OSWindow*                _window;
    bool                    _hasUserCreatedWindow;

protected:

    Stopwatch               m_graphicsWatch;
    Stopwatch               m_logicWatch;
    Stopwatch               m_networkWatch;
    Stopwatch               m_userInputWatch;
    Stopwatch               m_simulationWatch;
    Stopwatch               m_waitWatch;

    /** The original settings */
    const Settings          m_settings;

    /** onPose(), onGraphics(), and onWait() execute once ever m_renderPeriod 
        simulation frames. This allows UI/network/simulation to be clocked much faster
        than rendering to increase responsiveness. */
    int                     m_renderPeriod;

    WidgetManager::Ref      m_widgetManager;

    bool                    m_endProgram;
    int                     m_exitCode;

    /**
       Used to find the frame for defaultCamera.
    */
    Manipulator::Ref        m_cameraManipulator;

    GMutex                  m_debugTextMutex;

    /**
       Strings that have been printed with screenPrintf.
       Protected by m_debugTextMutex.
    */
    Array<std::string>      debugText;

    Color4                  m_debugTextColor;
    Color4                  m_debugTextOutlineColor;

    /**
       Processes all pending events on the OSWindow queue into the userInput.
       This is automatically called once per frame.  You can manually call it
       more frequently to get higher resolution mouse tracking or to prevent
       the OS from locking up (and potentially crashing) while in a lengthy
       onGraphics call.
    */
    virtual void processGEventQueue();

    static void staticConsoleCallback(const std::string& command, void* me);

    /** If true, configure 3D rendering to use m_frameBuffer and m_film in GApp::onGraphics. 
        m_film must be non-null to use this.

        \sa setFilmEnabled, filmEnabled, m_film
    */
    bool                    m_useFilm;

    /** Allocated if GApp::Settings::FilmSettings::enabled was true
        when the constructor executed.  Automatically resized by
        resize() when the screen size changes. 

        \sa setFilmEnabled, filmEnabled, m_useFilm
    */
    Film::Ref               m_film;

    DepthOfField::Ref       m_depthOfField;

    /** Framebuffer used for rendering the 3D portion of the scene. */
    Framebuffer::Ref        m_frameBuffer;

    /** Always bound to m_frameBuffer FrameBuffer::COLOR0. */
    Texture::Ref            m_colorBuffer0;

    /** Always bound to m_frameBuffer FrameBuffer::DEPTH. If NULL, the
        preferred depth format may not be supported by a Texture.  In
        that case, check m_depthRenderBuffer.*/
    Texture::Ref            m_depthBuffer;

    /** Always bound to m_frameBuffer FrameBuffer::DEPTH. This is used if
     the preferred depth format is not supported for Texture%s.*/
    Renderbuffer::Ref       m_depthRenderBuffer;

    /** Used to track how much onWait overshot its desired target during the previous frame. */
    RealTime                m_lastFrameOverWait;

    /** 
      Helper for generating cube maps.  Invokes GApp::onGraphics3D six times, once for each face of a cube map.  This is convenient both for microrendering
      and for generating cube maps to later use offline.

      G3D::Film post-processing is not applied to the resulting images.
  
      \param output If empty or the first element is NULL, this is set to a series of new 1024 x 1024 ImageFormat::RGB16F() textures.  Otherwise, the provided elements are used. 
      Textures are assumed to be square.  The images are generated in G3D::CubeFace order.

      \param camera The field of view is changed to 90 degrees and the view is rotated in 90 degree increments, but other parameters from this camera are used unmodifed.
      The app->defaultCamera is temporarily set to each face's camera as well, and then restored.

      \param depthMap Optional pre-allocated depth texture to use as the depth map when rendering each face.  Will be allocated to match the texture resolution if not provided.
      The default depth format is ImageFormat::DEPTH24().

      Example:
      \code
        Array<Texture::Ref> output;
        renderCubeMap(renderDevice, output, defaultCamera);

        GImage temp;
        const Texture::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);
        for (int f = 0; f < 6; ++f) {
            const Texture::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
            output[f]->getImage(temp, ImageFormat::RGB8());
            temp.flipVertical();
            temp.rotate90CW(-faceInfo.rotations);
            if (faceInfo.flipY) { temp.flipVertical();   }
            if (faceInfo.flipX) { temp.flipHorizontal(); }
            temp.save(format("cube-%s.png", faceInfo.suffix.c_str()));     
        }
        \endcode
    */
    virtual void renderCubeMap(RenderDevice* rd, Array<Texture::Ref>& output, GCamera camera, Texture::Ref depthMap = NULL);

public:

    /** Creates a default lighting environment for demos, which uses the file
    on the noonclouds/noonclouds_*.jpg textures.  The code that it uses is below.  Note that this loads
    a cube map every time that it is invoked, so this should not be used within the rendering loop.

  <pre>
    Lighting::Ref lighting = Lighting::create();
    lighting->shadowedLightArray.append(GLight::directional(Vector3(1,2,1), Color3::fromARGB(0xfcf6eb)));
    lighting->lightArray.append(GLight::directional(Vector3(-1,-0.5f,-1), Color3::fromARGB(0x1e324d)));
    lighting->environmentMap = 
        Texture::fromFile(pathConcat(System::findDataFile("sky"), "noonclouds/noonclouds_*.jpg"), 
                          TextureFormat::RGB8(), Texture::DIM_CUBE_MAP, Texture::Settings::cubeMap(),
                          Texture::Preprocess::gamma(2.1f));
    lighting->environmentMapColor = Color3::one();
   </pre>
   */
    static Lighting::Ref defaultLighting();

    /** Add your own debugging controls to this window.*/
    GuiWindow::Ref          debugWindow;
    /** debugWindow->pane() */
    GuiPane*                debugPane;

    void vscreenPrintf
    (const char*                 fmt,
     va_list                     argPtr) G3D_CHECK_VPRINTF_METHOD_ARGS;;

    void setFilmEnabled(bool e) {
        m_useFilm = e;
        debugAssertM(! m_useFilm || (m_useFilm && m_film.notNull()), 
                     "GApp::Settings::FilmSettings::enabled must be true when the GApp is constructed to later set GApp::m_useFilm = true");
    }

    bool filmEnabled() const {
        return m_useFilm;
    }

    const Stopwatch& graphicsWatch() const {
        return m_graphicsWatch;
    }

    const Stopwatch& waitWatch() const {
        return m_waitWatch;
    }

    const Stopwatch& logicWatch() const {
        return m_logicWatch;
    }

    const Stopwatch& networkWatch() const {
        return m_networkWatch;
    }

    const Stopwatch& userInputWatch() const {
        return m_userInputWatch;
    }

    const Stopwatch& simulationWatch() const {
        return m_simulationWatch;
    }

    /** Initialized to GApp::Settings::dataDir, or if that is "<AUTO>", 
        to System::demoFindData(). To make your program
        distributable, override the default 
        and copy all data files you need to a local directory.
        Recommended setting is "data/" or "./", depending on where
        you put your data relative to the executable.

        Your data directory must contain the default debugging font, 
        "console-small.fnt", unless you change it.
    */
    std::string             dataDir;

    RenderDevice*           renderDevice;

    /** Command console. */
    GConsoleRef             console;

    /** The window that displays buttons for debugging.  If GApp::Settings::useDeveloperTools is true
        this will be created and added as a Widget on the GApp.  Otherwise this will be NULL.
    */
    DeveloperWindow::Ref    developerWindow;

    /**
       NULL if not loaded
    */
    GFontRef                debugFont;
    UserInput*              userInput;

    /** Invoke to true to end the program at the end of the next event loop. */
    virtual void setExitCode(int code = 0);

    /**
       A default camera that is driven by the defaultController.
    */
    GCamera                    defaultCamera;

    /**
       Allows first person (Quake game-style) control
       using the arrow keys or W,A,S,D and the mouse.

       To disable, use:
       <pre>
       setCameraManipulator(NULL);
       m_widgetManager->remove(defaultController);
       defaultController = NULL;
       </pre>

    */
    FirstPersonManipulatorRef       defaultController;

    /**
       The manipulator that positions the defaultCamera every frame.
       By default, this is set to the defaultController.  This may be
       set to NULL to disable explicit camera positioning.
    */
    inline void setCameraManipulator(const Manipulator::Ref& man) {
        m_cameraManipulator = man;
    }

    Manipulator::Ref cameraManipulator() const {
        return m_cameraManipulator;
    }
    
    OSWindow* window() const {
        return _window;
    }

    /**
       When true, debugPrintf prints to the screen.
       (default is true)
    */
    bool                    showDebugText;

    enum Action {
        ACTION_NONE,
        ACTION_QUIT,
        ACTION_SHOW_CONSOLE
    };

    /**
       When true an GKey::ESCAPE keydown event
       quits the program.
       (default is true)
    */
    Action                  escapeKeyAction;

    /**
       When true GKey::F2 keydown deactivates
       the camera and restores the mouse cursor.
       (default is true). This works even if GApp::Settings::useDeveloperTools is false.
    */
    bool                    fastSwitchCamera;

    /**
       When true,   renderDebugInfo prints the frame rate and
       other data to the screen.
    */
    bool                    showRenderingStats;

    /**
       When true, the G3D::UserInput->beginEvents/endEvents processing is handled 
       for you by calling processGEventQueue() before G3D::GApp::onUserInput is called.  If you turn
       this off, you must call processGEventQueue() or provide your own event to userInput processing in onUserInput.
       (default is true)
    */
    bool                    manageUserInput;

    /**
       When true, there is an assertion failure if an exception is 
       thrown.

       Default is true.
    */
    bool                    catchCommonExceptions;

    /**
       Called from GApplet::run immediately after doGraphics to render
       the debugging text.  Does nothing if debugMode is false.  It
       is not usually necessary to override this method.
    */
    virtual void renderDebugInfo();

    /**
       @param window If null, a SDLWindow will be created for you. This
       argument is useful for substituting a different window
       system (e.g. GlutWindow)
    */
    GApp(const Settings& options = Settings(), OSWindow* window = NULL);

    virtual ~GApp();

    /**
       Call this to run the app.
    */
    int run();

    /** Draw a simple, short message in the center of the screen and swap the buffers. 
      Useful for loading screens and other slow operations.*/
    void drawMessage(const std::string& message);

    /** Displays the texture in a new GuiWindow */
    GuiWindow::Ref show(const Texture::Ref& t, const std::string& windowCaption = "");

    GuiWindow::Ref show(const ImageBuffer::Ref& t, const std::string& windowCaption = "") {
        return show(Texture::fromImageBuffer("", t), windowCaption);
    }

    /** \a T must be some Image class, e.g., Image3uint, Image4, etc.*/
    template<class T>
    GuiWindow::Ref show(const ReferenceCountedPointer<T>& t, const std::string& windowCaption = "") {
        return show(Texture::fromMemory("", t->getCArray(), t->format(), t->width(), t->height(), 1), windowCaption);
    }

private:

    /** Used by doSimulation for elapsed time. */
    RealTime               m_now, m_lastTime;

    /** Used by doWait for elapsed time. */
    RealTime               m_lastWaitTime;

    /** FPS for ideal time */
    float                  m_desiredFrameRate;

    /** \copydoc setLowerFramerateInBackground */
    bool                   m_lowerFrameRateInBackground;

    /** SPF for sim time */
    float                  m_simTimeStep;
    RealTime               m_realTime;
    SimTime                m_simTime;



protected:
    Array<SurfaceRef>   m_posed3D;
    Array<Surface2DRef> m_posed2D;

private:

    /** Helper for run() that actually starts the program loop. Called from run(). */
    void onRun();

    /**
       Initializes state at the beginning of onRun, including calling onCleanup.
    */
    void beginRun();

    /**
       Cleans up at the end of onRun, including calling onCleanup.
    */
    void endRun();

    /** 
        A single frame of rendering, simulation, AI, events, networking,
        etc.  Invokes the onXXX methods. 
    */
    void oneFrame();

public:

    /**
       Installs a module.  Actual insertion may be delayed until the next frame.
    */
    virtual void addWidget(const Widget::Ref& module, bool setFocus = true);

    /**
       The actual removal of the module may be delayed until the next frame.
    */
    virtual void removeWidget(const Widget::Ref& module);

    /** @brief Elapsed time per RENDERED frame for ideal simulation. Set to 0 to pause
        simulation, 1/fps to match real-time.  The actual sdt argument to
        onSimulation is simTimStep / m_renderPeriod.
    */
    float simTimeStep() const {
        return m_simTimeStep;
    }

    virtual void setSimTimeStep(float s);

    /** Accumulated wall-clock time since init was called on this applet. 
        Since this time is accumulated, it may drift from the true
        wall-clock obtained by System::time().*/
    RealTime realTime() const {
        return m_realTime;
    }

    virtual void setRealTime(RealTime r);

    /** In-simulation time since init was called on this applet.  
        Takes into account simTimeSpeed.  Automatically incremented
        after doSimulation.
    */
    SimTime simTime() const {
        return m_simTime;
    }

    virtual void setSimTime(SimTime s);

    /** Change to invoke frame limiting via doWait.
        Defaults to finf() */
    virtual void setDesiredFrameRate(float fps);

    /** If true, the desired frame rate is ignored when the OSWindow does not have focus 
     and the program switches to running 4fps.
    */
    virtual void setLowerFramerateInBackground(bool s) {
        m_lowerFrameRateInBackground = s;
    }

    bool lowerFrameRateInBackground() const {
        return m_lowerFrameRateInBackground;
    }

    float desiredFrameRate() const {
        return m_desiredFrameRate;
    }

    RealTime desiredFrameDuration() const {
        return 1.0 / m_desiredFrameRate;
    }

protected:

    /** Change the size of the underlying Film. Called by GApp::GApp() and GApp::onEvent(). This is not an event handler.  If you want 
      to be notified when your app is resized, override GApp::onEvent to handle the resize event (just don't forget to call GApp::onEvent as well) */
    void resize(int w, int h);

    /** Shorthand for developerWindow->cameraControlWindow->bookmark(name) */
    CoordinateFrame bookmark(const std::string& name, const CoordinateFrame& defaultValue = CoordinateFrame()) const;

    /**
       Load your data here.  Unlike the constructor, this catches common exceptions.
       It is called before the first frame is processed.
    */
    virtual void onInit();

    /**
       Unload/deallocate your data here.  Unlike the constructor, this catches common exceptions.
       It is called after the last frame is processed.
    */
    virtual void onCleanup();


    /**
       Override this with your simulation code.
       Called from GApp::run.
        
       Default implementation does nothing.

       simTime(), idealSimTime() and realTime() are incremented after
       doSimulation is called, so at the beginning of call the current
       time is the end of the previous frame.

       @param rdt Elapsed real-world time since the last call to doSimulation.

       @param sdt Elapsed sim-world time since the last call to
       doSimulation, computed by multiplying the wall-clock time by the
       simulation time rate.

       @param idt Elapsed ideal sim-world time.  Use this for perfectly
       reproducible timing results.  Ideal time always advances by the
       desiredFrameDuration * simTimeRate, no matter how much wall-clock
       time has elapsed.
    */
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    /** Invoked before onSimulation is run on the installed GModules and GApp.
        This is not used by most programs; it is primarily a hook for those performing
        extensive physical simulation on the GModules that need a setup and cleanup step.

        If you mutate the timestep arguments then those mutated time steps are passed
        to the onSimulation method.  However, the accumulated time will not be affected by
        the changed timestep.
    */
    virtual void onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt);

    /**
       Invoked after onSimulation is run on the installed GModules and GApp.
       Not used by most programs.
    */
    virtual void onAfterSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    /**
       Rendering callback used to paint the screen.  Called automatically.
       RenderDevice::beginFrame and endFrame are called for you before this 
       is invoked.

       The default implementation calls onGraphics2D and onGraphics3D as follows:
\code
void GApp::onGraphics(RenderDevice* rd, Array<SurfaceRef>& posed3D, Array<Surface2DRef>& posed2D) {
    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    // Clear the entire screen (needed even though we'll render over it because
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    if (m_useFilm) {
        // Clear the frameBuffer
        rd->pushState(m_frameBuffer);
        rd->clear();
        if (m_colorBuffer0->format()->floatingPoint) {
            // Float render targets don't support line smoothing
            rd->setMinLineWidth(1);
        }
        renderDevice->setMinLineWidth(1);
    } else {
        rd->pushState();
    }

    rd->setProjectionAndCameraMatrix(defaultCamera);
    onGraphics3D(rd, posed3D);

    rd->popState();
    if (m_useFilm) {
        // Expose the film
        m_film->exposeAndRender(rd, m_colorBuffer0, 1);
        rd->setMinLineWidth(0);
    }

    rd->push2D();
    {
        onGraphics2D(rd, posed2D);
    }
    rd->pop2D();
}
\endcode
     */
    virtual void onGraphics(RenderDevice* rd, Array<Surface::Ref>& surface, 
                            Array<Surface2D::Ref>& surface2D);

    /**
       Called from the default onGraphics.
       
       Override and implement. 
   */
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);

    /**
       \brief Called from the default onGraphics.
       Override and implement. 

    The RenderDevice will already be cleared and the default camera set when this method is invoked.
    By default, the Film's Framebuffer is bound and the output will be gamma corrected
    and bloomed.

    A common task is rendering SuperSurface%s with your own shader, which can be done by:
\code

    rd->pushState();
    rd->setShader(myShader)
    for (int s = 0; s < surface3D.size(); ++s) {
        const SuperSurface::Ref& surface = surface3D[s].downcast<SuperSurface>();
        if (surface.notNull()) {
            const SuperSurface::GPUGeom::Ref& geom = surface->gpuGeom();
            const SuperBSDF::Ref& bsdf = geom->material->bsdf();
            
            rd->setObjectToWorldMatrix(surface->coordinateFrame());
            
            myShader->args.set("lambertianConstant", bsdf->lambertian().constant());
            myShader->args.set("lambertianMap", Texture::whiteIfNull(bsdf->lambertian().texture()));
            ...
                
            rd->beginIndexedPrimitives();
            {
                rd->setNormalArray(geom->normal);
                rd->setVertexArray(geom->vertex);
                rd->setTexCoordArray(0, geom->texCoord0);
                rd->sendIndices(geom->primitive, geom->index);
            }
            rd->endIndexedPrimitives();
        }
    }
    rd->popState();
\endcode
   */
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);

    /** Called before onGraphics.  Append any models that you want
        rendered (you can also explicitly pose and render in your
        onGraphics method).  The provided arrays will already contain
        posed models from any installed Widgets. */
    virtual void onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);

    /**
       For a networked app, override this to implement your network
       message polling.
    */
    virtual void onNetwork();

    /**
       Task to be used for frame rate limiting.  

       Overriding onWait is not recommended unless you have significant
       computation tasks that cannot be executed conveniently on a separate thread.

       Frame rate limiting is useful
       to avoid overloading a maching that is running background tasks and
       for situations where fixed time steps are needed for simulation and there
       is no reason to render faster.

       Default implementation System::sleep()s on waitTime (which is always non-negative)
    */
    virtual void onWait(RealTime waitTime);


    /**
       Update any state you need to here.  This is a good place for
       AI code, for example.  Called after onNetwork and onUserInput,
       before onSimulation.
    */
    virtual void onAI();

    
    /**
       It is recommended to override onUserInput() instead of this method.

       Override if you need to explicitly handle events raw in the order
       they appear rather than once per frame by checking the current
       system state.
     
       Note that the userInput contains a record of all
       keys pressed/held, mouse, and joystick state, so 
       you do not have to override this method to handle
       basic input events.

       Return true if the event has been consumed (i.e., no-one else 
       including GApp should process it further).

       The default implementation does nothing.

       This runs after the m_widgetManager's onEvent, so a widget may consume
       events before the App sees them.
    */
    virtual bool onEvent(const GEvent& event);

    /**
       Routine for processing user input from the previous frame.  Default implementation does nothing.
    */
    virtual void onUserInput(class UserInput* userInput);

    /**
       Invoked when a user presses enter in the in-game console.  The default implementation
       ends the program if the command is "exit".


       Sample implementation:
       <pre>
        void App::onConsoleCommand(const std::string& str) {
            // Add console processing here

            TextInput t(TextInput::FROM_STRING, str);
            if (t.isValid() && (t.peek().type() == Token::SYMBOL)) {
                std::string cmd = toLower(t.readSymbol());
                if (cmd == "exit") {
                    setExitCode(0);
                    return;
                } else if (cmd == "help") {
                    printConsoleHelp();
                    return;
                }

                // Add commands here
            }

            console->printf("Unknown command\n");
            printConsoleHelp();
        }

        void App::printConsoleHelp() {
            console->printf("exit          - Quit the program\n");
            console->printf("help          - Display this text\n\n");
            console->printf("~/ESC         - Open/Close console\n");
            console->printf("F2            - Enable first-person camera control\n");
            console->printf("F4            - Record video\n");
        }
        </pre>
    */
    virtual void onConsoleCommand(const std::string& cmd);
};


/**
   Displays output on the last G3D::GApp instantiated.  If there was no GApp instantiated,
   does nothing.  Threadsafe.
   
   This is primarily useful for code that prints (almost) the same
   values every frame (e.g., "current position = ...") because those
   values will then appear in the same position on screen.

   For one-off print statements (e.g., "network message received")
   see G3D::consolePrintf.
 */
void screenPrintf(const char* fmt ...) G3D_CHECK_PRINTF_ARGS;

}

#endif
