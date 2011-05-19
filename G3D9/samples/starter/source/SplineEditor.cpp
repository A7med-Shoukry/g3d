#include "SplineEditor.h"


SplineEditor::Ref SplineEditor::create(const GuiText& caption, GuiTheme::Ref theme) {
    if (theme.isNull()) {
        theme = GuiTheme::lastThemeLoaded.createStrongPtr();
        alwaysAssertM(theme.notNull(), "Need a non-NULL GuiTheme for SplineEditor.");
    }
    
    return new SplineEditor(caption, theme);
}


SplineEditor::SplineEditor(const GuiText& caption, GuiTheme::Ref theme) : 
    GuiWindow(caption,
              theme,
              Rect2D::xywh(0,0,100,40), 
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    m_selectedControlPointIndex(0) {

    m_spline.append(CFrame());

    m_surface = new SplineSurface(this);
    m_nodeManipulator = ThirdPersonManipulator::create();
    m_nodeManipulator->setEnabled(false);


    GuiPane* p = pane();
    GuiPane* cpPane = p->addPane("Control Point");
    cpPane->addButton("Add new", this, &SplineEditor::addControlPoint);
    m_removeSelectedButton = cpPane->addButton("Remove selected", this, &SplineEditor::removeSelectedControlPoint);

    p->addCheckBox("Cyclic", Pointer<bool>(this, &SplineEditor::cyclic, &SplineEditor::setCyclic));
}


bool SplineEditor::cyclic() const {
    return m_spline.cyclic;
}


void SplineEditor::setCyclic(bool c) {
    m_spline.cyclic = c;
}


void SplineEditor::addControlPoint() {
    if (m_spline.control.size() == 0) {
        m_spline.append(CFrame());
    } else if (m_spline.control.size() == 1) {
        // Adding the 2nd point
        CFrame f = m_spline.control.last();
        f.translation += f.lookVector();
        m_spline.append(f);

        // Select the new point
        ++m_selectedControlPointIndex;
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
        ++m_selectedControlPointIndex;

        // Fix the rest of the times to be offset by the inserted duration
        float shift = newT - t0;
        for (int i = m_selectedControlPointIndex + 1; i < m_spline.time.size(); ++i) {
            m_spline.time[i] += shift;
        }
    }
}


void SplineEditor::removeSelectedControlPoint() {
    if (m_spline.control.size() <= 1) {
        // Can't delete!
        return;
    }

    // TODO: Should we fix the times?  Maybe
    m_spline.time.remove(m_selectedControlPointIndex);
    m_spline.control.remove(m_selectedControlPointIndex);
    m_selectedControlPointIndex = iClamp(m_selectedControlPointIndex - 1, 0, m_spline.control.size() - 1);
}


void SplineEditor::onPose(Array<Surface::Ref>& posedArray, Array<Surface2D::Ref>& posed2DArray) {
    if (enabled()) {
        posedArray.append(m_surface);
    }

    GuiWindow::onPose(posedArray, posed2DArray);
}


void SplineEditor::setManager(WidgetManager* m) {
    if ((m == NULL) && (manager() != NULL)) {
        // Remove controls from old manager
        manager()->remove(m_nodeManipulator);
    }

    GuiWindow::setManager(m);
    
    if (m != NULL) {
        m->add(m_nodeManipulator);
    }
}


void SplineEditor::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GuiWindow::onSimulation(rdt, sdt, idt);

    m_nodeManipulator->setEnabled(enabled());
    
    if (enabled()) {
        m_spline.control[m_selectedControlPointIndex] = m_nodeManipulator->frame();
        m_removeSelectedButton->setEnabled(m_spline.control.size() > 1);
    }
}


void SplineEditor::setSpline(const PhysicsFrameSpline& s) {
    m_spline = s;

    // Ensure that there is at least one node
    if (m_spline.control.size() == 0) {
        m_spline.control.append(PFrame());
    }

    m_selectedControlPointIndex = iClamp(m_selectedControlPointIndex, 0, m_spline.control.size() - 1);
    
    m_nodeManipulator->setFrame(m_spline.control[m_selectedControlPointIndex]);
}
