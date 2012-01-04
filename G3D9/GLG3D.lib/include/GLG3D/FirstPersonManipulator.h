/**
  \file GLG3D/FirstPersonManipulator.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2002-07-28
  \edited  2011-09-03
*/

#ifndef G3D_FirstPersonManipulator_h
#define G3D_FirstPersonManipulator_h

#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/Widget.h"

namespace G3D {

class CoordinateFrame;

typedef ReferenceCountedPointer<class FirstPersonManipulator> FirstPersonManipulatorRef;

/**
 Uses a First Person (Quake- or World of Warcraft style) mapping to translate keyboard and mouse input
 into a flying camera position.  The result is an Euler-angle 
 camera controller suitable for games and fly-throughs.  

  To use without G3D::GApp:
  <OL>
    <LI> Create a G3D::RenderDevice
    <LI> Create a UserInput object (set the keyboard controls when creating it)
    <LI> Create a ManualCameraController
    <LI> Call ManualCameraController::setActive(true)
    <LI> Invoke ManualCameraController::doSimulation every time simulation is invoked (e.g. once per rendering iteration)
    <LI> Use ManualCameraController::getCoordinateFrame() to set the camera's position
  </OL>

 */
class FirstPersonManipulator : public Manipulator {
public:
    enum MouseMode {
        /** Shooter/Quake style (default), mouse cursor is hidden and mouse controls yaw/pitch */
        MOUSE_DIRECT,

        /** RPG/World of Warcraft style, on right mouse button cursor
            is hidden and mouse controls yaw/pitch.  On OS X,
            ctrl+left button are treated as the right
            mouse button.*/
        MOUSE_DIRECT_RIGHT_BUTTON,

        /** Leaves mouse cursor visible and rotates
            while mouse is near the window edge. */
        MOUSE_SCROLL_AT_EDGE,

        /** RTS style. Leaves mouse cursor visible and rotates when
            the mouse is actively pushing against the window edge */
        MOUSE_PUSH_AT_EDGE
    };

    typedef ReferenceCountedPointer<FirstPersonManipulator> Ref;

private:

    GKey                        m_flyUpKey;
    GKey                        m_flyDownKey;

    /** m/s */
    float                       m_maxMoveRate;
    
    /** rad/s */
    float                       m_maxTurnRate;

    /** When the shift key is held down, multiply m_maxMoveRate by this.  Default is 0.1 */
    float                       m_shiftSpeedMultiplier;
    
    float                       m_yaw;
    float                       m_pitch;
    Vector3                     m_translation;

    bool                        m_active;

    class UserInput*            m_userInput;

    MouseMode                   m_mouseMode;

    /** Is the right mouse button currently held down? */
    bool                        m_rightDown;

    /** Returns true if the right mouse button is down, or on OSX, the
        user is using the left mouse and ctrl */
    bool rightDown(UserInput*) const;

    FirstPersonManipulator(UserInput* ui);

public:
    
    /** If the UserInput is not provided here, it is automatically recorded 
    after the first onUserInput method invocation */
    static FirstPersonManipulatorRef create(UserInput* ui = NULL);
        
    /** Deactivates the controller */
    virtual ~FirstPersonManipulator();

    /** When active, the FirstPersonManipulator takes over the mouse.  It turns
        off the mouse cursor and switches to first person controller style.
        Use this to toggle between your menu system and first person camera control.

        When deactivated, the mouse cursor is restored and the mouse is located
        where it was when the camera controller was activated.

        In release mode, the cursor movement is restricted to the window
        while the controller is active.  This does not occur in debug mode because
        you might hit a breakpoint while the controller is active and it
        would be annoying to not be able to move the mouse.*/
    void setActive(bool a);

    MouseMode mouseMode() const;

    void setMouseMode(MouseMode m);

    bool active() const;

    /** Initial value is 10 */
    void setMoveRate(double metersPerSecond);

    /** Initial value is PI / 2 */
    void setTurnRate(double radiansPerSecond);

    /** Invoke immediately before entering the main game loop. */
    void reset();

    /** \copydoc m_shiftSpeedMultipler */
    void setShiftSpeedMultipler(float m) {
        m_shiftSpeedMultiplier = m;
    }

    float shiftSpeedMultipler() const {
        return m_shiftSpeedMultiplier;
    }

    /** Key for translating +Y */
    GKey flyUpKey() const {
        return m_flyUpKey;
    }

    /** Key for translating -Y */
    GKey flyDownKey() const {
        return m_flyDownKey;
    }

    /** Default is 'z' */
    void setFlyUpKey(GKey k) {
        m_flyUpKey = k;
    }

    /** Default is 'c' */
    void setFlyDownKey(GKey k) {
        m_flyDownKey = k;
    }

    void setPosition(const Vector3& t) {
		m_translation = t;
	}

    void lookAt(const Vector3& position);

    float yaw() const {
        return m_yaw;
    }

    float pitch() const {
        return m_pitch;
    }

	const Vector3& translation() const {
		return m_translation;
	}

    /**
      Sets to the closest legal controller orientation to the coordinate frame.
     */
    void setFrame(const CoordinateFrame& c);

    // Inherited from Manipulator
    virtual void getFrame(CoordinateFrame& c) const;
    virtual CoordinateFrame frame() const;

    // Inherited from Widget
    virtual void onPose(Array<Surface::Ref>& p3d, Array<Surface2DRef>& p2d);
    virtual void onNetwork();
    virtual void onAI();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onUserInput(UserInput* ui);
    virtual bool onEvent(const GEvent& event);

    Vector3 lookVector() const {
        return frame().lookVector();
    }
};

}
#endif
