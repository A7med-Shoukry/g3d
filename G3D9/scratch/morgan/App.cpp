/** \file App.cpp */
#include "App.h"


//////////////////////////////////////////




class GuiCFrameBox : public GuiContainer {
private:

    Pointer<CFrame>         m_cframe;

    /** Cached to avoid recomputing for every draw call */
    CFrame                  m_lastCFrame;
    float                   m_yaw;
    float                   m_pitch;
    float                   m_roll;
    std::string             m_centerString;

    GuiTextBox*             m_centerBox;
    GuiNumberBox<float>*    m_yawBox;
    GuiNumberBox<float>*    m_pitchBox;
    GuiNumberBox<float>*    m_rollBox;

    GuiButton*              m_bookmarkButton;
    GuiButton*              m_showBookmarksButton;
    GuiButton*              m_copyButton;

    static const int        NUM_CHILDREN = 7;
    GuiControl*             m_child[NUM_CHILDREN];

    static const int        smallFontHeight = 7;

    /** A Any::TABLE shared over all instances. */
    static Any              m_bookmarks;
    
    /** Override m_cframe from the internal state variables. */
    void overrideCFrameValues();

    /** Update the internal state variables from m_cframe */
    void copyStateFromCFrame();
    
    void copyToClipboardButton();

    void setBookmark(const std::string& name, const CFrame& frame);
    void removeBookmark(const std::string& name);
    void onBookmarkButton();
    void saveBookmarks();
    void loadBookmarks();

public:

    GuiCFrameBox
       (const Pointer<CFrame>&  cframe,
        GuiContainer*           parent, 
        const GuiText&          caption);
    
    ~GuiCFrameBox();

    virtual void setCaption(const GuiText& c) override;
    
    virtual void setEnabled(bool e) override;

    virtual void setRect(const Rect2D& rect) override;

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override;

    virtual bool onChildControlEvent(const GEvent& e) override;

    virtual bool onEvent(const GEvent& e) override;

    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const override;
};


Any GuiCFrameBox::m_bookmarks;


/////////////////////////////////////////

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


void GuiCFrameBox::overrideCFrameValues() {
    try {
        TextInput t(TextInput::FROM_STRING, m_centerString);

        m_lastCFrame.translation.x = t.readNumber();
        t.readSymbol(",");
        m_lastCFrame.translation.y = t.readNumber();
        t.readSymbol(",");
        m_lastCFrame.translation.z = t.readNumber();
    } catch (...) {
        // Ignore parse errors
    }

    m_lastCFrame.rotation = Matrix3::fromEulerAnglesYXZ(toRadians(m_yaw), toRadians(m_pitch), toRadians(m_roll));
        
    *m_cframe = m_lastCFrame;
}


void GuiCFrameBox::copyStateFromCFrame() {
    m_lastCFrame = *m_cframe;        
    m_lastCFrame.rotation.toEulerAnglesYXZ(m_yaw, m_pitch, m_roll);
    m_yaw   = toDegrees(m_yaw);
    m_pitch = toDegrees(m_pitch);
    m_roll  = toDegrees(m_roll);
    m_centerString = format("%6.2f, %6.2f, %6.2f", 
        m_lastCFrame.translation.x, m_lastCFrame.translation.y, m_lastCFrame.translation.z);
}
    

void GuiCFrameBox::copyToClipboardButton() {        
    System::setClipboardText(m_lastCFrame.toXYZYPRDegreesString());
}


void GuiCFrameBox::setBookmark(const std::string& name, const CFrame& frame) {
    m_bookmarks[name] = frame;
    saveBookmarks();
}


void GuiCFrameBox::removeBookmark(const std::string& name) {
    if (m_bookmarks.containsKey(name)) {
        m_bookmarks.remove(name);
        saveBookmarks();
    }
}    

void GuiCFrameBox::saveBookmarks() {
    m_bookmarks.save("g3d-boomarks.any");
}


void GuiCFrameBox::loadBookmarks() {
    m_bookmarks = Any(Any::TABLE);
    if (FileSystem::exists("g3d-boomarks.any")) {
        m_bookmarks.load("g3d-boomarks.any");
    }
}



void GuiCFrameBox::onBookmarkButton() {
    std::string name;
    BookmarkDialog::Result result = BookmarkDialog::RESULT_CANCEL;

    BookmarkDialogRef dialog = 
        new BookmarkDialog(m_gui->window(), rect().center() + Vector2(0, 100), 
         theme(), name, result, m_lastCFrame.toXYZYPRDegreesString());

    dialog->showModal(m_gui->window());

    dialog = NULL;

    switch (result) {
    case BookmarkDialog::RESULT_CANCEL:
        break;

    case BookmarkDialog::RESULT_OK:
        setBookmark(name, m_lastCFrame);
        break;

    case BookmarkDialog::RESULT_DELETE:
        removeBookmark(name);
        break;
    }
}


GuiCFrameBox::GuiCFrameBox
    (const Pointer<CFrame>&  cframe,
    GuiContainer*           parent, 
    const GuiText&          caption) : 
    GuiContainer(parent, caption), m_cframe(cframe) {

    copyStateFromCFrame();
    loadBookmarks();

    static const float rotationControlWidth = 52;
    static const float captionWidth = 10;
    static const float rotationPrecision = 0.1f;
    static const float translationPrecision = 0.001f;
    static const std::string degrees = "\xba";
    static const float unitsSize = 8.0;
    GuiNumberBox<float>* c = NULL;

    m_centerBox = new GuiTextBox(this, "", &m_centerString, GuiTextBox::DELAYED_UPDATE, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE);
    m_centerBox->setSize(Vector2(160, CONTROL_HEIGHT));

    m_yawBox = new GuiNumberBox<float>(this, "", &m_yaw, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
    m_yawBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
    c = m_yawBox; c->setCaptionWidth(0);  c->setUnitsSize(unitsSize);

    m_pitchBox = new GuiNumberBox<float>(this, "", &m_pitch, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
    m_pitchBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
    c = m_pitchBox;  c->setCaptionWidth(0); c->setUnitsSize(unitsSize);

    m_rollBox = new GuiNumberBox<float>(this, "", &m_roll, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
    m_rollBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
    c = m_rollBox; c->setCaptionWidth(0); c->setUnitsSize(unitsSize);

    // Change to black "r" (x) for remove
    const char* DOWN = "6";
    const char* CHECK = "\x98";
    const char* CLIPBOARD = "\xA4";
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    m_bookmarkButton = new GuiButton(this, GuiControl::Callback(this, &GuiCFrameBox::onBookmarkButton), 
            GuiText(CHECK, iconFont, 16, Color3::blue() * 0.8f), 
            GuiTheme::TOOL_BUTTON_STYLE);

    m_showBookmarksButton = new GuiButton(this, GuiButton::Callback(), GuiText(DOWN, iconFont, 18), GuiTheme::TOOL_BUTTON_STYLE);

    m_copyButton = new GuiButton(this, GuiControl::Callback(this, &GuiCFrameBox::copyToClipboardButton), GuiText(CLIPBOARD, iconFont, 16), GuiTheme::TOOL_BUTTON_STYLE);
#   ifdef G3D_OSX
        m_copyButton->setEnabled(false);
#   endif

    float w = 18;
    float h = 21;
    m_showBookmarksButton->setSize(w, h);
    m_bookmarkButton->setSize(w, h);
    m_copyButton->setSize(w, h);

    m_child[0] = m_centerBox;
    m_child[1] = m_yawBox;
    m_child[2] = m_pitchBox;
    m_child[3] = m_rollBox;
    m_child[4] = m_bookmarkButton;
    m_child[5] = m_showBookmarksButton;
    m_child[6] = m_copyButton;

    // Slightly taller than most controls because of the labels
    setRect(Rect2D::xywh(0, 0, 500, CONTROL_HEIGHT + smallFontHeight));
}
    

GuiCFrameBox:: ~GuiCFrameBox() {
    for (int c = 0; c < NUM_CHILDREN; ++c) {
        delete m_child[c];
    }
}


void GuiCFrameBox::setCaption(const GuiText& c) {
    GuiContainer::setCaption(c);

    // Resize other parts in response to caption size changing
    setRect(m_rect);
}


void GuiCFrameBox::setEnabled(bool e) {
    for (int c = 0; c < NUM_CHILDREN; ++c) {
        m_child[c]->setEnabled(e);
    }
}


void GuiCFrameBox::setRect(const Rect2D& rect) {
    GuiContainer::setRect(rect);

    // Total size of the GUI, after the caption
    float controlSpace = m_rect.width() - m_captionWidth;

    // Position the children
    m_centerBox->setPosition(m_captionWidth, 0);
    m_yawBox->moveRightOf(m_centerBox);
    m_yawBox->moveBy(10, 0);
    m_pitchBox->moveRightOf(m_yawBox);
    m_rollBox->moveRightOf(m_pitchBox);

    m_showBookmarksButton->moveRightOf(m_rollBox);
    m_showBookmarksButton->moveBy(0, 2);
    m_bookmarkButton->moveRightOf(m_showBookmarksButton);
    m_copyButton->moveRightOf(m_bookmarkButton);

}


void GuiCFrameBox::findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
    if (! m_clientRect.contains(mouse) || ! m_visible || ! m_enabled) {
        return;
    }

    mouse -= m_clientRect.x0y0();
    for (int c = 0; c < NUM_CHILDREN; ++c) {
        m_child[c]->findControlUnderMouse(mouse, control);
    }
}


bool GuiCFrameBox::onChildControlEvent(const GEvent& e) {
    if (e.type == GEventType::GUI_ACTION) {

        if ((e.gui.control == m_centerBox) ||
                (e.gui.control == m_yawBox) ||
                (e.gui.control == m_rollBox) ||
                (e.gui.control == m_pitchBox)) {

            // One of the text boxes changed.  Update the underlying CFrame
            overrideCFrameValues();

            // Fire my own action event
            GEvent response;
            response.type = GEventType::GUI_ACTION;
            response.gui.control = this;
            m_gui->fireEvent(response);
        }

        // Hide the child event from other windows
        return true;
    }

    return false;
}


bool GuiCFrameBox::onEvent(const GEvent& e)  {
    if (GuiContainer::onEvent(e)) {
        return true;
    }

    return false;
}


void GuiCFrameBox::render(RenderDevice* rd, const GuiThemeRef& skin) const {
    // Ensure that we're in sync with the CFrame
    if (m_lastCFrame != *m_cframe) {
        const_cast<GuiCFrameBox*>(this)->overrideCFrameValues();
    }

    static const float smallFontSize = 8;

    if (! m_visible) {
        return;
    }
    skin->pushClientRect(m_clientRect); {
        // Render caption
        skin->renderLabel(m_rect - m_clientRect.x0y0() - Vector2(0, 4), m_caption, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER, m_enabled);

        // Render the canvas surrounding the individual text boxes            
        skin->renderCanvas(Rect2D::xyxy(m_centerBox->rect().x0y0(), m_rollBox->rect().x1y1() + Vector2(2, 0)), m_enabled, false, "", 0);

        // Render child controls
        for (int c = 0; c < NUM_CHILDREN; ++c) {
            m_child[c]->render(rd, skin);
        }

        static const GuiText label[NUM_CHILDREN] = 
        {GuiText("center", NULL, smallFontSize),
         GuiText("yaw", NULL, smallFontSize),
         GuiText("pitch", NULL, smallFontSize),
         GuiText("roll", NULL, smallFontSize)};

        // Render labels
        for (int c = 0; c < 4; ++c) {
            skin->renderLabel(m_child[c]->rect() + Vector2((c > 0) ? -4 : 0, smallFontHeight), label[c], GFont::XALIGN_CENTER, GFont::YALIGN_BOTTOM, m_enabled);
        }        
    } skin->popClientRect();
}


/////////////////////////////////////////////////////








// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();


/** Dumps the geometry and texture coordinates (no materials) to a
    file.  Does not deal with nested parts */
void convertToOBJFile(const std::string& srcFilename) {
    const std::string dstFilename = FilePath::base(srcFilename) + ".obj";

    FILE* file = FileSystem::fopen(dstFilename.c_str(), "wt");

    ArticulatedModel::Ref m = ArticulatedModel::fromFile(srcFilename);

    {
        int tri, vert;
        m->countTrianglesAndVertices(tri, vert);
        debugPrintf("%d triangles, %d vertices\nGenerating OBJ...\n", tri, vert);
    }

    fprintf(file, "# %s\n\n", m->name.c_str());
    for (int p = 0; p < m->rootArray().size(); ++p) {
        const ArticulatedModel::Part* part = m->rootArray()[p];

        const CFrame& cframe = part->cframe;
        
        // Number of vertices
        const int N = part->cpuVertexArray.size();

        // Construct a legal part name
        std::string name = "";
        for (int i = 0; i < (int)part->name.size(); ++i) {
            const char c = part->name[i];
            if (isDigit(c) || isLetter(c)) {
                name += c;
            } else {
                name += "_";
            }
        }

        if (name == "") {
            name = format("UnnamedPart%d", p);
        }

        // Part name
        fprintf(file, "\ng %s \n", name.c_str());

        // Write geometry.  Compress the data by only writing
        // unique values in each of the v, vt, vn arrays,
        // and using %g for output.
        fprintf(file, "\n");
            
        Table<Point3, int> vertexToVertexIndex;
        Table<int, int> vertexIndexToVertexIndex;
        int numVertices = 0;
        for (int v = 0; v < N; ++v) {
            const Point3& vertex = part->cpuVertexArray.vertex[v].position;
            bool created = false;
            int& vertexIndex = vertexToVertexIndex.getCreate(vertex, created);
            if (created) {
                const Point3& transformed = cframe.pointToWorldSpace(vertex);
                fprintf(file, "v %g %g %g\n", transformed.x, transformed.y, transformed.z);
                vertexIndex = numVertices; 
                ++numVertices;
            }
            vertexIndexToVertexIndex.set(v, vertexIndex);
        }
                        
        bool hasTexCoords = part->hasTexCoord0();
        Table<Point2, int> texCoordToTexCoordIndex;
        Table<int, int> texCoordIndexToTexCoordIndex;
        int numTexCoords = 0;
        if (hasTexCoords) {
            // Make sure there really are useful (nonzero) texture coordinates
            hasTexCoords = false;
            for (int v = 0; v < N; ++v) {
                if (! part->cpuVertexArray.vertex[v].texCoord0.isZero()) {
                    hasTexCoords = true;
                    break;
                }
            }

            fprintf(file, "\n");
            for (int v = 0; v < N; ++v) {
                const Point2& texCoord = part->cpuVertexArray.vertex[v].texCoord0;
                bool created = false;
                int& texCoordIndex = texCoordToTexCoordIndex.getCreate(texCoord, created);
                if (created) {
                    // G3D's texture coordinate convention is upside down of OBJ's
                    fprintf(file, "vt %g %g\n", texCoord.x, 1.0f - texCoord.y);
                    texCoordIndex = numTexCoords;
                    ++numTexCoords;
                }
                texCoordIndexToTexCoordIndex.set(v, texCoordIndex);
            }
        }

        fprintf(file, "\n");
        Table<Vector3, int> normalToNormalIndex;
        Table<int, int> normalIndexToNormalIndex;
        int numNormals = 0;
        for (int v = 0; v < N; ++v) {
            const Vector3& normal = part->cpuVertexArray.vertex[v].normal;
            bool created = false;
            int& normalIndex = normalToNormalIndex.getCreate(normal, created);
            if (created) {
                const Vector3& transformed = cframe.vectorToWorldSpace(normal);
                fprintf(file, "vn %g %g %g\n", transformed.x, transformed.y, transformed.z);
                normalIndex = numNormals;
                ++numNormals;
            }
            normalIndexToNormalIndex.set(v, normalIndex);
        }

        // Triangle list
        fprintf(file, "\n");
        for (int t = 0; t < part->meshArray().size(); ++t) {
            const ArticulatedModel::Mesh* mesh = part->meshArray()[t];
            alwaysAssertM(mesh->primitive == PrimitiveType::TRIANGLES, "Only triangle lists supported");
            for (int i = 0; i < mesh->cpuIndexArray.size(); i += 3) {
                fprintf(file, "f");
                for (int j = 0; j < 3; ++j) {
                    // Vertex index in the original mesh
                    const int index = mesh->cpuIndexArray[i + j];

                    // Indices are 1-based; negative values
                    // reference relative to the last vertex
                    // added.

                    if (hasTexCoords) {
                        fprintf(file, " %d/%d/%d", 
                                vertexIndexToVertexIndex[index] - numVertices, 
                                texCoordIndexToTexCoordIndex[index] - numTexCoords, 
                                normalIndexToNormalIndex[index] - numNormals);
                    } else {
                        fprintf(file, " %d//%d",
                                vertexIndexToVertexIndex[index] - numVertices, 
                                normalIndexToNormalIndex[index] - numNormals);
                    }
                }
                fprintf(file, "\n");
            }
        }
    }
    
    FileSystem::fclose(file);
}


int main(int argc, const char* argv[]) {
    OSWindow* osWindow = OSWindow::create();
    //GLCaps::init();

    RenderDevice* rd = new RenderDevice();
    rd->init(osWindow);
    
    glClear(GL_COLOR_BUFFER_BIT);
    debugAssertGLOk();
    ::exit(0);

#if 0
    Array<std::string> s;
    VideoOutput::getSupportedCodecs(s);

#ifdef G3D_NO_FFMPEG
    printf("G3D_NO_FFMPEG (app): %d\n", G3D_NO_FFMPEG);
#else
    printf("G3D_NO_FFMPEG (app): (undefined)\n");
#endif


    printf("G3D_NO_FFMPEG (library): %s\n", 
           VideoOutput::ENABLED_IN_LIBRARY ? "(undefined)" : "(defined!)");

    printf("Supported Codecs:\n");
    for (int i = 0; i < s.length(); ++i) { 
        printf("%s\n", s[i].c_str());
    }
    if (s.length() == 0) {
        printf("(none)\n");
    }
    ::exit(0);
#endif
   
    // Make Gui
//    GuiTheme::makeThemeFromSourceFiles	("D:/morgan/g3d/data-source/guithemes/osx-10.7/", "osx-10.7_white.png", "osx-10.7_black.png", "osx-10.7.gtm.any", "D:/morgan/g3d/data-source/guithemes/osx-10.7/osx-10.7.gtm");
//    ::exit(0);
    /*
    // Make fonts
    GFont::makeFont(256, "d:/font/LucidaSans", "d:/font/LucidaSans.fnt"); 
    GFont::makeFont(256, "d:/font/LucidaSansItalic", "d:/font/LucidaSansItalic.fnt"); 
    GFont::makeFont(256, "d:/font/LucidaSansBold", "d:/font/LucidaSansBold.fnt"); 
    GFont::makeFont(256, "d:/font/LucidaSansBoldItalic", "d:/font/LucidaSansBoldItalic.fnt"); 
    ::exit(0);
    */

    /*
    // Gaussian coefficients:
    int N = 9;
    Array<float> coeff;
    float stddev = (2*N +1) * 0.25f;
    gaussian1D(coeff, (2*N +1), stddev);
    for (int i = 0; i < coeff.size(); ++i) 
        debugPrintf("weight[%d] = %f; ", i, coeff[i]);
    debugPrintf("\n");
    ::exit(0);
*/
    /*

    Array<std::string> files;
    FileSystem::getFiles("C:/Users/morgan/Desktop/san-miguel-export/Maps/*", files, true);

    for (int i = 0; i < files.size(); ++i) {
        debugPrintf("%s\n", files[i].c_str());
        GImage im(files[i]);
    }
    ::exit(0);

    */

    (void)argc; (void)argv;
    GApp::Settings settings(argc, argv);
    
    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.width       = 1280; 
    settings.window.height      = 720;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    renderDevice->setColorClearValue(Color3::white());
//    convertToOBJFile("dragon.ifs"); ::exit(0);
}




void App::onInit() {
    // Turn on the developer HUD
    debugWindow->setVisible(true);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;
//    debugPane->addLabel("asdasd as das das dsa dsa dasd   asdasdsad as dasd asdsa ")->setWidth(100);

    GuiPane* p = debugPane->addPane("Choices", GuiTheme::ORNATE_PANE_STYLE);
    static int x = 0;
    p->addRadioButton("Minimum", 0, &x);
    p->addRadioButton("Average", 1, &x);
    p->addRadioButton("Weighted Average", 2, &x);
    p->addRadioButton("Maximum", 3, &x);
    p->pack();

    debugPane->addButton("Hi");

    static CFrame C;
    debugPane->addCustom(new GuiCFrameBox(Pointer<CFrame>(&defaultCamera, &GCamera::coordinateFrame, &GCamera::setCoordinateFrame), debugPane, "CFrame"));
    
    lighting = defaultLighting();

    Texture::Settings cszSettings = Texture::Settings::buffer();
    cszSettings.interpolateMode   = Texture::NEAREST_MIPMAP;
    cszSettings.maxMipMap         = 5;
    texture = Texture::createEmpty("texture", 512, 512, ImageFormat::RG32F(), Texture::DIM_2D_NPOT, cszSettings);
    /*
    // Force MIP-map allocation
    glBindTexture(texture->openGLTextureTarget(), texture->openGLID());
    glGenerateMipmap(texture->openGLTextureTarget());
    glBindTexture(texture->openGLTextureTarget(), GL_NONE);
    */

    framebuffer = Framebuffer::create("framebuffer");
    framebuffer->set(Framebuffer::COLOR0, texture, CubeFace::POS_X, 0);

    framebuffer1 = Framebuffer::create("framebuffer");
    framebuffer1->set(Framebuffer::COLOR0, texture, CubeFace::POS_X, 1);

    //texture = Texture::fromFile(System::findDataFile("checkerboard.jpg"), ImageFormat::AUTO(), Texture::DIM_2D_NPOT, Texture::Settings::buffer());
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

void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);

    rd->push2D(framebuffer1); {
        rd->clear();
    } rd->pop2D();
//    model->pose(surface3D);
    Surface::sortAndRender(rd, defaultCamera, surface3D, lighting);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {

    rd->setTexture(0, texture);
    Draw::rect2D(Rect2D::xywh(0,0,400,400), rd, Color3::white(), Rect2D::xywh(-1,-1,3,3));

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
