#ifndef G3D_SplineEditor_h
#define G3D_SplineEditor_h

#include <G3D/G3DAll.h>

/** The spline manipulator is a GuiWindow that displays its controls.
    It also creates additional Widgets.

    Invoking setVisible(false) on the SplineEditor hides the
    control window, but not the 3D controls.  Use setEnabled(false)
    to hide the 3D controls.

    TODO: 
    - Set control point selection
    - Debug rendering of spline
    - Allow better control over time of control points
*/
class SplineEditor : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class SplineEditor> Ref;

protected:

    Surface::Ref                 m_surface;
    PhysicsFrameSpline           m_spline;
    int                          m_selectedControlPointIndex;
    ThirdPersonManipulator::Ref  m_nodeManipulator;
    GuiButton*                   m_removeSelectedButton;

    class SplineSurface : public EmptySurface {
    public:
        SplineEditor* m_manipulator;
        
        SplineSurface(SplineEditor* m) : m_manipulator(m) {}
        
        virtual void render(RenderDevice* rd) const {
            Draw::physicsFrameSpline(m_manipulator->m_spline, rd);
        }
        
        virtual std::string name() const {
            return "SplineEditor";
        }
        
        virtual void getCoordinateFrame(CoordinateFrame& c) const {
            c = CFrame();
        }
    };
    
    SplineEditor(const GuiText& caption, GuiTheme::Ref theme);
        
public:

    static Ref create(const GuiText& caption = "Spline", GuiTheme::Ref theme = NULL);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray);

    virtual void setManager(WidgetManager* m);

    /** Gui Callback */
    void addControlPoint();

    void removeSelectedControlPoint();

    int selectedControlPointIndex() const {
        return m_selectedControlPointIndex;
    }

    virtual void setSelectedControlPointIndex(int i) {
        m_selectedControlPointIndex = iClamp(i, 0, m_spline.control.size() - 1);
    }
    
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

#endif // SplineEditor_h
