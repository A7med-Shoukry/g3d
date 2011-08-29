/**
 \file GLG3D/PhysicsFrameSplineEditor.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-06-05
 \edited  2011-08-29

 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_PhysicsFrameSplineEditor_h
#define G3D_PhysicsFrameSplineEditor_h

#include "GLG3D/GuiWindow.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/Surface.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/GuiNumberBox.h"

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
    bool                         m_isDocked;

    GuiRadioButton*              m_finalIntervalChoice[2];
    GuiNumberBox<float>*         m_finalIntervalBox;

    /** Used to avoid constantly unparsing the current physics frame in selectedNodePFrameAsString() */
    mutable PhysicsFrame         m_cachedPhysicsFrameValue;
    mutable std::string          m_cachedPhysicsFrameString;

    class SplineSurface : public Surface {
    public:
        PhysicsFrameSplineEditor* m_manipulator;
        
        SplineSurface(PhysicsFrameSplineEditor* m) : m_manipulator(m) {}
        
        virtual void render(RenderDevice* rd) const;
        
        virtual std::string name() const {
            return "PhysicsFrameSplineEditor";
        }
        
        virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const {
            c = CFrame();
        }

        virtual void getObjectSpaceBoundingBox(G3D::AABox& b, bool previous = false) const {
            b = AABox::inf();
        }

        virtual void getObjectSpaceBoundingSphere(G3D::Sphere& s, bool previous = false) const {
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
    
    PhysicsFrameSplineEditor(const GuiText& caption, GuiPane* dockPane, GuiTheme::Ref theme);
        
public:

    /** \param dockPane If not NULL, the 2D GUI is placed into this pane and no visible window is created */
    static Ref create(const GuiText& caption = "Spline Editor", GuiPane* dockPane = NULL, GuiTheme::Ref theme = NULL);

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray);

    virtual void setManager(WidgetManager* m);

    /** Gui Callback */
    void addControlPoint();

    void removeSelectedControlPoint();

    int selectedControlPointIndex() const {
        return m_selectedControlPointIndex;
    }

    /** Used by the GUI. */
    std::string selectedNodePFrameAsString() const;

    /** Used by the GUI. */
    void setSelectedNodePFrameFromString(const std::string& s);

    /** Used by the GUI. */
    float selectedNodeTime() const;

    /** Used by the GUI. */
    void setSelectedNodeTime(float t);
    
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
        if (e && ! m_isDocked) {
            setVisible(true);
        }
    }

};

} // G3D

#endif // PhysicsFrameSplineEditor_h
