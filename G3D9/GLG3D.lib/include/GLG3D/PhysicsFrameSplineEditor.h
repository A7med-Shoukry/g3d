#ifndef G3D_PhysicsFrameSplineEditor_h
#define G3D_PhysicsFrameSplineEditor_h

#include "GLG3D/GuiWindow.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/Surface.h"
#include "G3D/PhysicsFrameSpline.h"

namespace G3D {

/** 2D/3D control for editing PhysicsFrameSpline%s.

    The spline manipulator is a GuiWindow that displays its controls.
    It also creates additional Widget%s.

    Invoking setVisible(false) on the PhysicsFrameSplineEditor hides the
    control window, but not the 3D controls.  Use setEnabled(false)
    to hide the 3D controls.

    \beta 

    TODO: 
    - Set control point selection with mouse
    - Debug rendering of spline
    - Allow better control over time of control points (maybe auto-time based on distance?)
    - Show time graph with GuiFunction box?
    - Show camera-control style editing of control points
    - Allow transformation of an entire spline via a checkbox
*/
class PhysicsFrameSplineEditor : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class PhysicsFrameSplineEditor> Ref;

protected:

    Surface::Ref                 m_surface;
    PhysicsFrameSpline           m_spline;
    int                          m_selectedControlPointIndex;
    ThirdPersonManipulator::Ref  m_nodeManipulator;
    GuiButton*                   m_removeSelectedButton;

    class SplineSurface : public Surface {
    public:
        PhysicsFrameSplineEditor* m_manipulator;
        
        SplineSurface(PhysicsFrameSplineEditor* m) : m_manipulator(m) {}
        
        virtual void render(RenderDevice* rd) const;
        
        virtual std::string name() const {
            return "PhysicsFrameSplineEditor";
        }
        
        virtual void getCoordinateFrame(CoordinateFrame& c, float timeOffset = 0.0f) const {
            c = CFrame();
        }

        virtual void getObjectSpaceBoundingBox(G3D::AABox& b, float timeOffset = 0.0f) const {
            b = AABox::inf();
        }

        virtual void getObjectSpaceBoundingSphere(G3D::Sphere& s, float timeOffset = 0.0f) const {
            s = Sphere(Point3::zero(), finf());
        }

        virtual void sendGeometry(RenderDevice* rd) const {
            alwaysAssertM(false, "Not implemented");
        }
    protected:
        virtual void defaultRender(RenderDevice* rd) const {
            alwaysAssertM(false, "Not implemented");
        }
    };
    
    PhysicsFrameSplineEditor(const GuiText& caption, GuiTheme::Ref theme);
        
public:

    static Ref create(const GuiText& caption = "Spline Editor", GuiTheme::Ref theme = NULL);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray);

    virtual void setManager(WidgetManager* m);

    /** Gui Callback */
    void addControlPoint();

    void removeSelectedControlPoint();

    int selectedControlPointIndex() const {
        return m_selectedControlPointIndex;
    }

    virtual void setSelectedControlPointIndex(int i);
    
    /** Returns true if the underlying spline is cyclic */
    bool cyclic() const;

    /** Sets the cyclic property of the underlying spline */
    void setCyclic(bool c);

    const PhysicsFrameSpline& spline() const {
        return m_spline;
    }

    virtual void setSpline(const PhysicsFrameSpline& p);

    virtual void setEnabled(bool e) {
        GuiWindow::setEnabled(e);

        // If enabled, also make visible (so that the window can be seen)
        if (e) {
            setVisible(true);
        }
    }

};

} // G3D

#endif // PhysicsFrameSplineEditor_h
