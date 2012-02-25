/**
  \file CameraControlWindow.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-06-01
  \edited  2010-03-01
*/
#include "G3D/platform.h"
#include "G3D/GCamera.h"
#include "G3D/prompt.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"
#include "G3D/FileSystem.h"


namespace G3D {

enum {FILM_PANE_SIZE = 102, FOCUS_PANE_SIZE = 54};
const Vector2 CameraControlWindow::sDefaultWindowSize(286 + 16, 46);
const Vector2 CameraControlWindow::sExpandedWindowSize(286 + 16, 176 + FILM_PANE_SIZE + FOCUS_PANE_SIZE);

static const std::string noSpline = "< None >";
static const std::string untitled = "< Unsaved >";

typedef ReferenceCountedPointer<class BookmarkDialog> BookmarkDialogRef;
class BookmarkDialog : public GuiWindow {
public:
    enum Result {RESULT_OK, RESULT_CANCEL, RESULT_DELETE};

    bool               ok;

    Result&            m_result;
    std::string&       m_name;
    std::string        m_originalName;
    GuiButton*         m_okButton;
    GuiButton*         m_deleteButton;

    /** Name */
    GuiTextBox*        m_textBox;

    OSWindow*          m_osWindow;
    
    BookmarkDialog(OSWindow* osWindow, const Vector2& position, GuiThemeRef skin, 
                   std::string& name, Result& result,
                   const std::string& note) : 
        GuiWindow("Bookmark Properties", skin, Rect2D::xywh(position - Vector2(160, 0), Vector2(300, 100)), 
                  GuiTheme::DIALOG_WINDOW_STYLE, GuiWindow::NO_CLOSE),
        m_result(result),
        m_name(name),
        m_originalName(name) {

        m_textBox = pane()->addTextBox("Name", &name, GuiTextBox::IMMEDIATE_UPDATE);
        
        GuiLabel* loc = pane()->addLabel("Location");
        loc->setWidth(84);
        GuiLabel* locDisplay = pane()->addLabel(note);
        locDisplay->moveRightOf(loc);

        m_okButton = pane()->addButton("Ok");
        m_okButton->moveBy(130, 20);
        m_deleteButton = pane()->addButton("Delete");
        m_deleteButton->moveRightOf(m_okButton);
        setRect(Rect2D::xywh(rect().x0y0(), Vector2::zero()));
        pane()->setSize(0, 0);
        pane()->pack();
        sync();

        m_textBox->setFocused(true);
    }

protected:

    void close(Result r) {
        setVisible(false);
        m_manager->remove(this);
        m_result = r;
    }

    /** Update enables/captions */
    void sync() {
        if ((m_originalName != m_name) || (m_name == "")) {
            m_deleteButton->setCaption("Cancel");
        }

        m_okButton->setEnabled(m_name != "");
    }

public:

    virtual bool onEvent(const GEvent& e) {
        if (GuiWindow::onEvent(e)) {
            return true;
        }

        sync();

        if (e.type == GEventType::GUI_ACTION) {
            if (e.gui.control == m_okButton) {
                close(RESULT_OK);
                return true;
            } else if (e.gui.control == m_deleteButton) {
                if (m_deleteButton->caption().text() == "Cancel") {
                    close(RESULT_CANCEL);
                } else {
                    close(RESULT_DELETE);
                }
                return true;
            }
        }

        if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
            close(RESULT_CANCEL);
            return true;
        }

        return false;
    }

};


void CameraControlWindow::setFocusZ(float z) {
    m_camera->setFocusPlaneZ(z);
}

float CameraControlWindow::focusZ() const {
    return m_camera->focusPlaneZ();
}

void CameraControlWindow::setLensRadius(float r) {
    m_camera->setLensRadius(r);
}

float CameraControlWindow::lensRadius() const {
    return m_camera->lensRadius();
}

static bool hasRoll = false;


/** Negates a numeric type G3D::Pointer value */
template<class T>
class NegativeAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<T>;

    Pointer<T>      m_source;

    typedef ReferenceCountedPointer<NegativeAdapter> Ptr;

    NegativeAdapter(Pointer<T> ptr) : m_source(ptr) {}

    /** For use by Pointer<T> */
    T get() const {  return -m_source.getValue();     }

    /** For use by Pointer<T> */
    void set(const T& v) {   m_source.setValue(-v);   }

public:

    static Pointer<T> create(Pointer<T> ptr) {
        Ptr p = new NegativeAdapter(ptr);
        return Pointer<T>(p, &NegativeAdapter<T>::get, &NegativeAdapter<T>::set);
    }
};

CameraControlWindow::CameraControlWindow(
    const FirstPersonManipulatorRef&      manualManipulator, 
    const UprightSplineManipulatorRef&    trackManipulator, 
    const Pointer<Manipulator::Ref>&      cameraManipulator,
    GCamera*                              camera,
    const Film::Ref&                      film,
    const GuiThemeRef&                    skin) : 
    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(5, 54, 200, 0),
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    m_trackFileIndex(0),
    m_bookmarkSelection(NO_BOOKMARK),
    m_camera(camera),
    m_cameraManipulator(cameraManipulator),
    m_manualManipulator(manualManipulator),
    m_trackManipulator(trackManipulator),
    m_drawerButton(NULL),
    m_drawerButtonPane(NULL),
    m_expanded(false)
{

    m_manualOperation = m_manualManipulator->active();

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    const char* DOWN = "6";
    const char* CHECK = "\x98";
    const char* CLIPBOARD = "\xA4";
    float w = 18;
    float h = 20;
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    // The default G3D textbox label doesn't support multiple fonts
    if (hasRoll) {
        pane->addLabel(GuiText("q q q", greekFont, 12))->setRect(Rect2D::xywh(19, 6, 10, 15));
        pane->addLabel(GuiText("y  x  z", NULL, 9))->setRect(Rect2D::xywh(24, 12, 10, 9));
    } else {
        pane->addLabel(GuiText("q q", greekFont, 12))->setRect(Rect2D::xywh(19, 6, 10, 15));
        pane->addLabel(GuiText("y  x", NULL, 9))->setRect(Rect2D::xywh(24, 12, 10, 9));
    }
    m_cameraLocationTextBox = 
        pane->addTextBox("xyz",
        Pointer<std::string>(this, &CameraControlWindow::cameraLocation, 
                                   &CameraControlWindow::setCameraLocation));
    m_cameraLocationTextBox->setRect(Rect2D::xywh(0, 2, 246 + (hasRoll ? 20.0f : 0.0f), 24));
    m_cameraLocationTextBox->setCaptionWidth(38 + (hasRoll ? 12.0f : 0.0f));

    // Change to black "r" (x) for remove
    GuiButton* bookMarkButton = 
        pane->addButton(GuiText(CHECK, iconFont, 16, Color3::blue() * 0.8f), 
            GuiControl::Callback(this, &CameraControlWindow::onBookmarkButton),
            GuiTheme::TOOL_BUTTON_STYLE);
    bookMarkButton->setSize(w, h);
    bookMarkButton->moveRightOf(m_cameraLocationTextBox);
    bookMarkButton->moveBy(-2, 2);

    m_showBookmarksButton = pane->addButton(GuiText(DOWN, iconFont, 18), 
        GuiTheme::TOOL_BUTTON_STYLE);

    m_showBookmarksButton->setSize(w, h);

    GuiButton* copyButton = pane->addButton(GuiText(CLIPBOARD, iconFont, 16), GuiControl::Callback(this, &CameraControlWindow::copyToClipboard), GuiTheme::TOOL_BUTTON_STYLE);
    copyButton->setSize(w, h);
#   ifdef G3D_OSX
        copyButton->setEnabled(false);
#   endif

    /////////////////////////////////////////////////////////////////////////////////////////
    const float sliderWidth = 290,  indent = 2.0f;

    GuiPane* focusPane = pane->addPane();
    focusPane->moveBy(-9, 2);
    
    GuiControl* n = focusPane->addNumberBox("Focus", 
        NegativeAdapter<float>::create(Pointer<float>(this, &CameraControlWindow::focusZ, &CameraControlWindow::setFocusZ)),
        "m", GuiTheme::LOG_SLIDER, 0.01f, 200.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = focusPane->addNumberBox("Lens Radius", 
        Pointer<float>(this, &CameraControlWindow::lensRadius, &CameraControlWindow::setLensRadius),
        "m", GuiTheme::LOG_SLIDER, 0.0f, 0.5f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    /////////////////////////////////////////////////////////////////////////////////////////

    GuiPane* filmPane = pane->addPane();
    filmPane->moveBy(-9, 2);
    if (film.notNull()) {
        film->makeGui(filmPane, 10.0f, sliderWidth, indent);
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    GuiPane* manualPane = pane->addPane();
    manualPane->moveBy(-8, 0);

    manualPane->addCheckBox("Manual Control (F2)", &m_manualOperation)->moveBy(-2, 3);

    manualPane->beginRow();
    {
        m_trackList = manualPane->addDropDownList("Path", m_trackFileArray, &m_trackFileIndex);
        m_trackList->setRect(Rect2D::xywh(Vector2(0, m_trackList->rect().y1() - 25), Vector2(180, m_trackList->rect().height())));
        m_trackList->setCaptionWidth(34);

        m_visibleCheckBox = manualPane->addCheckBox("Visible", 
            Pointer<bool>(trackManipulator, 
                          &UprightSplineManipulator::showPath, 
                          &UprightSplineManipulator::setShowPath));

        m_visibleCheckBox->moveBy(6, 0);
    }
    manualPane->endRow();
    
    manualPane->beginRow();
    {
        Vector2 buttonSize = Vector2(20, 20);
        m_recordButton = manualPane->addRadioButton
            (GuiText::Symbol::record(), 
             UprightSplineManipulator::RECORD_KEY_MODE, 
             trackManipulator.pointer(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_recordButton->moveBy(32, 2);
        m_recordButton->setSize(buttonSize);
    
        m_playButton = manualPane->addRadioButton
            (GuiText::Symbol::play(), 
             UprightSplineManipulator::PLAY_MODE, 
             trackManipulator.pointer(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_playButton->setSize(buttonSize);

        m_stopButton = manualPane->addRadioButton
            (GuiText::Symbol::stop(), 
             UprightSplineManipulator::INACTIVE_MODE, 
             trackManipulator.pointer(),
             &UprightSplineManipulator::mode,
             &UprightSplineManipulator::setMode,
             GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        m_stopButton->setSize(buttonSize);

        m_saveButton = manualPane->addButton("Save...");
        m_saveButton->setSize(m_saveButton->rect().wh() - Vector2(20, 1));
        m_saveButton->moveBy(20, -3);
        m_saveButton->setEnabled(false);

        m_cyclicCheckBox = manualPane->addCheckBox("Cyclic", 
            Pointer<bool>(trackManipulator, 
                          &UprightSplineManipulator::cyclic, 
                          &UprightSplineManipulator::setCyclic));

        m_cyclicCheckBox->setPosition(m_visibleCheckBox->rect().x0(), m_saveButton->rect().y0() + 1);
    }
    manualPane->endRow();
    /*
    static float m_playbackSpeed = 1.0f;
    GuiNumberBox<float>* speedBox = manualPane->addNumberBox("Speed", &m_playbackSpeed, "x", GuiTheme::LOG_SLIDER, 0.1f, 10.0f);
    speedBox->setPosition(m_stopButton->rect().x0(), speedBox->rect().y0());
    speedBox->setCaptionSize(40);
    speedBox->setWidth(130);
    */

#   ifdef G3D_OSX
        m_manualHelpCaption = GuiText("W,A,S,D,Z,C keys and right mouse (or ctrl+left mouse) to move; SHIFT = slow", NULL, 10);
#   else
        m_manualHelpCaption = GuiText("W,A,S,D,Z,C keys and right mouse to move; SHIFT = slow", NULL, 10);
#   endif

    m_autoHelpCaption = "";
    m_playHelpCaption = "";

    m_recordHelpCaption = GuiText("Spacebar to place a control point.", NULL, 10);

    m_helpLabel = manualPane->addLabel(m_manualHelpCaption);
    m_helpLabel->moveBy(0, -4);

    manualPane->pack();
    filmPane->setWidth(manualPane->rect().width());
    pack();

    // Set width to max expanded size so client size is correct when adding drawer items below
    setRect(Rect2D::xywh(rect().x0y0(), sExpandedWindowSize));

    // Make the pane width match the window width
    manualPane->setPosition(0, manualPane->rect().y0());
    manualPane->setSize(clientRect().width(), manualPane->rect().height());

    // Have to create the m_drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    m_drawerCollapseCaption = GuiText("5", iconFont);
    m_drawerExpandCaption = GuiText("6", iconFont);
    m_drawerButtonPane = pane->addPane("", GuiTheme::NO_PANE_STYLE);
    m_drawerButton = m_drawerButtonPane->addButton(m_drawerExpandCaption, GuiTheme::TOOL_BUTTON_STYLE);
    m_drawerButton->setRect(Rect2D::xywh(0, 0, 12, 10));
    m_drawerButtonPane->setSize(12, 10);
    
    // Resize the pane to include the drawer button so that it is not clipped
    pane->setSize(clientRect().wh());

    setBookmarkFile("g3d-bookmarks.any");        
    if (m_bookmarkName.size() == 0) {
        // Make a default home bookmark
        m_bookmarkName.append("Home");
        m_bookmarkPosition.append(CoordinateFrame::fromXYZYPRDegrees(0,1,7,0,-15,0));
    }

    // Collapse the window back down to default size
    setRect(Rect2D::xywh(rect().x0y0(), sDefaultWindowSize));
    sync();
}


CameraControlWindow::Ref CameraControlWindow::create(
    const FirstPersonManipulatorRef&   manualManipulator,
    const UprightSplineManipulatorRef& trackManipulator,
    const Pointer<Manipulator::Ref>&   cameraManipulator,
    GCamera*                           camera,
    const Film::Ref&                   film,
    const GuiThemeRef&                 skin) {

    return new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, camera, film, skin);
}


void CameraControlWindow::setManager(WidgetManager* manager) {
    GuiWindow::setManager(manager);
    if (manager) {
        // Move to the upper right
        float osWindowWidth = manager->window()->dimensions().width();
        setRect(Rect2D::xywh(osWindowWidth - rect().width(), 40, rect().width(), rect().height()));
    }
}


std::string CameraControlWindow::cameraLocation() const {
    CoordinateFrame cframe;
    m_trackManipulator->camera()->getCoordinateFrame(cframe);
    UprightFrame uframe(cframe);
    
    // \xba is the character 186, which is the degree symbol
    return format("(% 5.1f, % 5.1f, % 5.1f), % 5.1f\xba, % 5.1f\xba", 
                  uframe.translation.x, uframe.translation.y, uframe.translation.z, 
                  toDegrees(uframe.yaw), toDegrees(uframe.pitch));
}


std::string CameraControlWindow::cameraLocationCode() const {
    CoordinateFrame cframe;
    m_trackManipulator->camera()->getCoordinateFrame(cframe);
    return cframe.toXYZYPRDegreesString();
}



void CameraControlWindow::setCameraLocation(const std::string& s) {
    TextInput t(TextInput::FROM_STRING, s);
    try {
        UprightFrame uframe;
        Token first = t.peek();
        if (first.string() == "CFrame") {
            // Code version
            t.readSymbols("CFrame", "::", "fromXYZYPRDegrees");
            t.readSymbol("(");
            uframe.translation.x = t.readNumber();
            t.readSymbol(",");
            uframe.translation.y = t.readNumber();
            t.readSymbol(",");
            uframe.translation.z = t.readNumber();
            t.readSymbol(",");
            uframe.yaw = toRadians(t.readNumber());
            t.readSymbol(",");
            uframe.pitch = toRadians(t.readNumber());
        } else {
            // Pretty-printed version
            uframe.translation.deserialize(t);
            t.readSymbol(",");
            uframe.yaw = toRadians(t.readNumber());
            std::string DEGREE = "\xba";
            if (t.peek().string() == DEGREE) {
                t.readSymbol();
            }
            t.readSymbol(",");
            uframe.pitch = toRadians(t.readNumber());
            if (t.peek().string() == DEGREE) {
                t.readSymbol();
            }
        }        
        CoordinateFrame cframe = uframe;

        m_trackManipulator->camera()->setCoordinateFrame(cframe);
        m_manualManipulator->setFrame(cframe);

    } catch (const TextInput::TokenException& e) {
        // Ignore the incorrectly formatted value
        (void)e;
    }
}


void CameraControlWindow::copyToClipboard() {
    System::setClipboardText(cameraLocationCode());
}


void CameraControlWindow::showBookmarkList() {
    if (m_bookmarkName.size() > 0) {
        m_menu = GuiMenu::create(theme(), &m_bookmarkName, &m_bookmarkSelection);
        manager()->add(m_menu);
        m_menu->show(manager(), this, NULL, m_cameraLocationTextBox->toOSWindowCoords(m_cameraLocationTextBox->clickRect().x0y1() + Vector2(45, 8)), false);
    }
}


void CameraControlWindow::onBookmarkButton() {
    std::string name;
    BookmarkDialog::Result result = BookmarkDialog::RESULT_CANCEL;

    BookmarkDialogRef dialog = 
        new BookmarkDialog(m_manager->window(), rect().center() + Vector2(0, 100), 
                           theme(), name, result, cameraLocation());

    dialog->showModal(m_manager->window());

    dialog = NULL;

    switch (result) {
    case BookmarkDialog::RESULT_CANCEL:
        break;

    case BookmarkDialog::RESULT_OK:
        {
            CoordinateFrame frame;
            m_trackManipulator->camera()->getCoordinateFrame(frame);
            setBookmark(name, frame);
        }
        break;

    case BookmarkDialog::RESULT_DELETE:
        removeBookmark(name);
        break;
    }
}


void CameraControlWindow::saveBookmarks() {
    Any all(Any::TABLE);
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        all[m_bookmarkName[i]] = m_bookmarkPosition[i];
    }
    all.save(m_bookmarkFilename);
}


void CameraControlWindow::setBookmarkFile(const std::string& filename) {
    m_bookmarkPosition.clear();
    m_bookmarkName.clear();
    m_bookmarkFilename = filename;

    if (FileSystem::exists(m_bookmarkFilename)) {
        // Load bookmarks
        Any all;
        try {
            all.load(m_bookmarkFilename);
            all.verifyType(Any::TABLE);
        } catch (...) {
            msgBox(m_bookmarkFilename + " is corrupt.");
            return;
        }

        all.table().getKeys(m_bookmarkName);
        m_bookmarkPosition.resize(m_bookmarkName.size());
        for (int i = 0; i < m_bookmarkName.size(); ++i) {
            m_bookmarkPosition[i] = all[m_bookmarkName[i]];
        }
    }
}


void CameraControlWindow::setBookmark(const std::string& name, 
                                      const CoordinateFrame& frame) {
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            m_bookmarkPosition[i] = frame;
            saveBookmarks();
            return;
        }
    }

    m_bookmarkName.append(name);
    m_bookmarkPosition.append(frame);
    saveBookmarks();
}


void CameraControlWindow::removeBookmark(const std::string& name) {
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            m_bookmarkName.remove(i);
            m_bookmarkPosition.remove(i);
            saveBookmarks();
            return;
        }
    }
}


CoordinateFrame CameraControlWindow::bookmark
(const std::string& name, 
 const CoordinateFrame& defaultValue) const {

    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            return m_bookmarkPosition[i];
        }
    }
    return defaultValue;
}


void CameraControlWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (m_drawerButtonPane) {
        const Rect2D& r = clientRect();
        m_drawerButtonPane->setPosition((r.width() - m_drawerButtonPane->rect().width()) / 2.0f, r.height() - m_drawerButtonPane->rect().height());
    }
}


void CameraControlWindow::updateTrackFiles() {
    m_trackFileArray.fastClear();
    m_trackFileArray.append(noSpline);
    FileSystem::getFiles("*.us.any", m_trackFileArray);

    // Element 0 is <unsaved>, so skip it
    for (int i = 1; i < m_trackFileArray.size(); ++i) {
        m_trackFileArray[i] = FilePath::base(FilePath::base(m_trackFileArray[i]));
    }
    m_trackFileIndex = iMin(m_trackFileArray.size() - 1, m_trackFileIndex);
}


void CameraControlWindow::onUserInput(UserInput* ui) {
    GuiWindow::onUserInput(ui);

    if (m_manualOperation && (m_trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE)) {
        // Keep the FPS controller in sync with the spline controller
        CoordinateFrame cframe;
        m_trackManipulator->getFrame(cframe);
        m_manualManipulator->setFrame(cframe);
        m_trackManipulator->camera()->setCoordinateFrame(cframe);
    }

    if (m_bookmarkSelection != NO_BOOKMARK) {
        // Clicked on a bookmark
        const CoordinateFrame& cframe = m_bookmarkPosition[m_bookmarkSelection];
        m_trackManipulator->camera()->setCoordinateFrame(cframe);
        m_manualManipulator->setFrame(cframe);

        m_bookmarkSelection = NO_BOOKMARK;
        // TODO: change "bookmark" caption to "edit"
    }
}


bool CameraControlWindow::onEvent(const GEvent& event) {

    // Allow super class to process the event
    if (GuiWindow::onEvent(event)) {
        return true;
    }
    
    // Accelerator key for toggling camera control.  Active even when the window is hidden.
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F2)) {
        m_manualOperation = ! m_manualOperation;
        sync();
        return true;
    }

    if (! visible()) {
        return false;
    }

    // Special buttons
    if (event.type == GEventType::GUI_ACTION) {
        GuiControl* control = event.gui.control;

        if ((control == m_showBookmarksButton) && (m_menu.isNull() || ! m_menu->visible())) {
            showBookmarkList();
            return true;
        } else if (control == m_drawerButton) {

            // Change the window size
            m_expanded = ! m_expanded;
            morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? sExpandedWindowSize : sDefaultWindowSize));
            m_drawerButton->setCaption(m_expanded ? m_drawerCollapseCaption : m_drawerExpandCaption);

        } else if (control == m_trackList) {
            
            if (m_trackFileArray[m_trackFileIndex] != untitled) {
                // Load the new spline
                loadSpline(m_trackFileArray[m_trackFileIndex] + ".us.any");

                // When we load, we lose our temporarily recorded spline,
                // so remove that display from the menu.
                if (m_trackFileArray.last() == untitled) {
                    m_trackFileArray.remove(m_trackFileArray.size() - 1);
                }
            }

        } else if (control == m_playButton) {

            // Take over manual operation
            m_manualOperation = true;
            // Restart at the beginning of the path
            m_trackManipulator->setTime(0);

        } else if ((control == m_recordButton) || (control == m_cameraLocationTextBox)) {

            // Take over manual operation and reset the recording
            m_manualOperation = true;
            m_trackManipulator->clear();
            m_trackManipulator->setTime(0);

            // Select the untitled path
            if ((m_trackFileArray.size() == 0) || (m_trackFileArray.last() != untitled)) {
                m_trackFileArray.append(untitled);
            }
            m_trackFileIndex = m_trackFileArray.size() - 1;

            m_saveButton->setEnabled(true);

        } else if (control == m_saveButton) {

            // Save
            std::string saveName;

            if (FileDialog::create(this)->getFilename(saveName)) {
                saveName = filenameBaseExt(trimWhitespace(saveName));

                if (saveName != "") {
                    saveName = saveName.substr(0, saveName.length() - filenameExt(saveName).length());
                    saveSpline(saveName);
                }
            }
        }
        sync();

    } else if (m_trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE) {
        // Check if the user has added a point yet
        sync();
    }

    return false;
}

void CameraControlWindow::saveSpline(const std::string& trackName) {
    Any any(m_trackManipulator->spline());
    any.save(trackName + ".us.any");

    updateTrackFiles();

    // Select the one we just saved
    m_trackFileIndex = iMax(0, m_trackFileArray.findIndex(trackName));
                    
    m_saveButton->setEnabled(false);
}

void CameraControlWindow::loadSpline(const std::string& filename) {
    m_saveButton->setEnabled(false);
    m_trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);

    if (filename == noSpline) {
        m_trackManipulator->clear();
        return;
    }

    if (! FileSystem::exists(filename)) {
        m_trackManipulator->clear();
        return;
    }

    Any any;
    any.load(filename);

    UprightSpline spline(any);

    m_trackManipulator->setSpline(spline);
    m_manualOperation = true;
}


void CameraControlWindow::sync() {

    if (m_expanded) {
        bool hasTracks = m_trackFileArray.size() > 0;
        m_trackList->setEnabled(hasTracks);

        bool hasSpline = m_trackManipulator->splineSize() > 0;
        m_visibleCheckBox->setEnabled(hasSpline);
        m_cyclicCheckBox->setEnabled(hasSpline);
        m_playButton->setEnabled(hasSpline);

        if (m_manualOperation) {
            switch (m_trackManipulator->mode()) {
            case UprightSplineManipulator::RECORD_KEY_MODE:
            case UprightSplineManipulator::RECORD_INTERVAL_MODE:
                m_helpLabel->setCaption(m_recordHelpCaption);
                break;

            case UprightSplineManipulator::PLAY_MODE:
                m_helpLabel->setCaption(m_playHelpCaption);
                break;

            case UprightSplineManipulator::INACTIVE_MODE:
                m_helpLabel->setCaption(m_manualHelpCaption);
            }
        } else {
            m_helpLabel->setCaption(m_autoHelpCaption);
        }
    }

    if (m_manualOperation) {
        // User has control
        bool playing = m_trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE;
        m_manualManipulator->setActive(! playing);
        if (playing) {
            *m_cameraManipulator = m_trackManipulator;
        } else {
            *m_cameraManipulator = m_manualManipulator;
        }
    } else {
        // Program has control
        m_manualManipulator->setActive(false);
        *m_cameraManipulator = Manipulator::Ref(NULL);
        m_trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
    }
}

}
