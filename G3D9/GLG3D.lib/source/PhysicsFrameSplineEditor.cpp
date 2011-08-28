/**
  \file PhysicsFrameSplineEditor.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2011-05-19
  \edited  2011-05-19
*/
#include "GLG3D/PhysicsFrameSplineEditor.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Draw.h"

namespace G3D {

void PhysicsFrameSplineEditor::SplineSurface::render(RenderDevice* rd) const {
    Draw::physicsFrameSpline(m_manipulator->m_spline, rd);
}


PhysicsFrameSplineEditor::Ref PhysicsFrameSplineEditor::create(const GuiText& caption, GuiPane* dockPane, GuiTheme::Ref theme) {
    if (theme.isNull()) {
        theme = GuiTheme::lastThemeLoaded.createStrongPtr();
        alwaysAssertM(theme.notNull(), "Need a non-NULL GuiTheme for PhysicsFrameSplineEditor.");
    }
    
    return new PhysicsFrameSplineEditor(caption, dockPane, theme);
}


PhysicsFrameSplineEditor::PhysicsFrameSplineEditor(const GuiText& caption, GuiPane* dockPane, GuiTheme::Ref theme) : 
    GuiWindow(caption,
              theme,
              Rect2D::xywh(0,0,100,40), 
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    m_selectedControlPointIndex(0),
    m_isDocked(dockPane != NULL) {

    m_cachedPhysicsFrameString = CFrame(m_cachedPhysicsFrameValue).toAny().unparse();
    m_spline.append(CFrame());

    m_surface = new SplineSurface(this);
    m_nodeManipulator = ThirdPersonManipulator::create();
    m_nodeManipulator->setEnabled(false);

    GuiPane* p = dockPane;

    if (p == NULL) {
        // Place into the window
        p = pane();
    } else {
        // No need to show the window
        setVisible(false);
    }
    
    GuiPane* cpPane = p->addPane("Control Point", GuiTheme::ORNATE_PANE_STYLE);
    cpPane->moveBy(0, -15);

    cpPane->addLabel("Control point: 0");
    cpPane->addNumberBox("Time", Pointer<float>(this, &PhysicsFrameSplineEditor::selectedNodeTime, &PhysicsFrameSplineEditor::setSelectedNodeTime), "s");
    cpPane->addTextBox("", Pointer<std::string>(this, &PhysicsFrameSplineEditor::selectedNodePFrameAsString, &PhysicsFrameSplineEditor::setSelectedNodePFrameFromString));

    cpPane->beginRow(); {
        GuiButton* b = cpPane->addButton("Add new", this, &PhysicsFrameSplineEditor::addControlPoint);
        b->moveBy(-2, -7);
        m_removeSelectedButton = cpPane->addButton("Remove", this, &PhysicsFrameSplineEditor::removeSelectedControlPoint);
    } cpPane->endRow();
    cpPane->pack();

    GuiControl* prev = p->addCheckBox("Cycle with ", Pointer<bool>(this, &PhysicsFrameSplineEditor::cyclic, &PhysicsFrameSplineEditor::setCyclic));

    GuiPane* finalIntervalPane = p->addPane("", GuiTheme::NO_PANE_STYLE);
    finalIntervalPane->moveRightOf(prev);
    finalIntervalPane->moveBy(0, -2);
    static int m_explicitFinalInterval = 0;
    finalIntervalPane->addRadioButton("automatic final interval", 0, &m_explicitFinalInterval);
    finalIntervalPane->beginRow(); {
        finalIntervalPane->addRadioButton("", 1, &m_explicitFinalInterval);
        finalIntervalPane->addNumberBox("", &m_spline.finalInterval, "s interval");
    } finalIntervalPane->endRow();

    pack();

    setEnabled(false);
}


float PhysicsFrameSplineEditor::selectedNodeTime() const {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {
        return m_spline.time[m_selectedControlPointIndex];
    } else {
        return 0.0f;
    }
}


void PhysicsFrameSplineEditor::setSelectedNodeTime(float t) {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {
        m_spline.time[m_selectedControlPointIndex] = t;
    }
}


std::string PhysicsFrameSplineEditor::selectedNodePFrameAsString() const {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {

        const PhysicsFrame& pframe = m_spline.control[m_selectedControlPointIndex];

        // Cache the string so that we don't have to reparse it for every rendering
        if (m_cachedPhysicsFrameValue != pframe) {
            m_cachedPhysicsFrameValue = pframe;
            m_cachedPhysicsFrameString = CFrame(m_cachedPhysicsFrameValue).toAny().unparse();
        }

        return m_cachedPhysicsFrameString;

    } else {

        return "Point3(0, 0, 0)";

    }
}


void PhysicsFrameSplineEditor::setSelectedNodePFrameFromString(const std::string& s) {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {
        try {
            const PFrame& pframe = Any::parse(s);
            m_spline.control[m_selectedControlPointIndex] = pframe;

            // Update the manipulator, so that it doesn't just override the value that we changed
            m_nodeManipulator->setFrame(pframe);
        } catch (...) {
            // Ignore parse errors
        }
    }
}


bool PhysicsFrameSplineEditor::cyclic() const {
    return m_spline.cyclic;
}


void PhysicsFrameSplineEditor::setCyclic(bool c) {
    m_spline.cyclic = c;
}


void PhysicsFrameSplineEditor::addControlPoint() {
    if (m_spline.control.size() == 0) {
        m_spline.append(CFrame());
    } else if (m_spline.control.size() == 1) {
        // Adding the 2nd point
        CFrame f = m_spline.control.last();
        f.translation += f.lookVector();
        m_spline.append(f);

        // Select the new point
        setSelectedControlPointIndex(m_selectedControlPointIndex + 1);

    } else {
        // Adding between two points
        float t0 = m_spline.time[m_selectedControlPointIndex];
        float newT = t0;
        float evalT = 0;
        if (m_selectedControlPointIndex < m_spline.control.size() - 1) {
            // Normal interval
            newT = m_spline.time[m_selectedControlPointIndex + 1];
            evalT = (t0 + evalT) / 2;
        } else if (m_spline.cyclic) {
            // After the end on a cyclic spline
            evalT = m_spline.finalInterval + t0;
            evalT = (t0 + evalT) / 2;
        } else {
            // After the end on a non-cyclic spline of length at least
            // 2; assume that we want to step the distance of the previous
            // interval
            newT = evalT = 2.0f * t0 - m_spline.time[m_selectedControlPointIndex - 1];
        }

        const PhysicsFrame f = m_spline.evaluate(evalT);
        m_spline.control.insert(m_selectedControlPointIndex, f);
        m_spline.time.insert(m_selectedControlPointIndex, newT);

        // Select the new point
        setSelectedControlPointIndex(m_selectedControlPointIndex + 1);

        // Fix the rest of the times to be offset by the inserted duration
        float shift = newT - t0;
        for (int i = m_selectedControlPointIndex + 1; i < m_spline.time.size(); ++i) {
            m_spline.time[i] += shift;
        }
    }
}


void PhysicsFrameSplineEditor::removeSelectedControlPoint() {
    if (m_spline.control.size() <= 1) {
        // Can't delete!
        return;
    }

    // TODO: Should we fix the times?  Maybe
    m_spline.time.remove(m_selectedControlPointIndex);
    m_spline.control.remove(m_selectedControlPointIndex);
    setSelectedControlPointIndex(m_selectedControlPointIndex - 1);
}


void PhysicsFrameSplineEditor::onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray) {
    if (enabled()) {
        posedArray.append(m_surface);
    }

    GuiWindow::onPose(posedArray, posed2DArray);
}


void PhysicsFrameSplineEditor::setManager(WidgetManager* m) {
    if ((m == NULL) && (manager() != NULL)) {
        // Remove controls from old manager
        manager()->remove(m_nodeManipulator);
    }

    GuiWindow::setManager(m);
    
    if (m != NULL) {
        m->add(m_nodeManipulator);
    }
}


void PhysicsFrameSplineEditor::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GuiWindow::onSimulation(rdt, sdt, idt);

    m_nodeManipulator->setEnabled(enabled());
    
    if (enabled()) {
        m_spline.control[m_selectedControlPointIndex] = m_nodeManipulator->frame();
        m_removeSelectedButton->setEnabled(m_spline.control.size() > 1);
    }
}


void PhysicsFrameSplineEditor::setSpline(const PhysicsFrameSpline& s) {
    m_spline = s;

    // Ensure that there is at least one node
    if (m_spline.control.size() == 0) {
        m_spline.control.append(PFrame());
    }

    m_selectedControlPointIndex = iClamp(m_selectedControlPointIndex, 0, m_spline.control.size() - 1);
    
    m_nodeManipulator->setFrame(m_spline.control[m_selectedControlPointIndex]);
}


void PhysicsFrameSplineEditor::setSelectedControlPointIndex(int i) {
    m_selectedControlPointIndex = iClamp(i, 0, m_spline.control.size() - 1);
    // Move the manipulator to the new control point
    m_nodeManipulator->setFrame(m_spline.control[m_selectedControlPointIndex]);
}

}
