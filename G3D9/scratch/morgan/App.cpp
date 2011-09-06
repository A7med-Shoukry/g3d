/** \file App.cpp */
#include "App.h"




// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();


/** Dumps the geometry and texture coordinates (no materials) to a
    file.  Does not deal with nested parts */
void convertToOBJFile(const std::string& srcFilename) {
    const std::string dstFilename = FilePath::base(srcFilename) + ".obj";

    FILE* file = FileSystem::fopen(dstFilename.c_str(), "wt");

    ArticulatedModel2::Ref m = ArticulatedModel2::fromFile(srcFilename);

    {
        int tri, vert;
        m->countTrianglesAndVertices(tri, vert);
        debugPrintf("%d triangles, %d vertices\nGenerating OBJ...\n", tri, vert);
    }

    fprintf(file, "# %s\n\n", m->name.c_str());
    for (int p = 0; p < m->rootArray().size(); ++p) {
        const ArticulatedModel2::Part* part = m->rootArray()[p];

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
            const ArticulatedModel2::Mesh* mesh = part->meshArray()[t];
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

    /*

    Array<std::string> files;
    FileSystem::getFiles("C:/Users/morgan/Desktop/san-miguel-export/Maps/*", files, true);

    for (int i = 0; i < files.size(); ++i) {
        debugPrintf("%s\n", files[i].c_str());
        GImage im(files[i]);
    }
    ::exit(0);

    */

    std::string d = "D:\\morgan\\foo";
    FilePath::parent(d);

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


class GuiCFrameBox : public GuiContainer {
private:

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

public:

    GuiCFrameBox
       (GuiContainer*           parent, 
        const GuiText&          caption) : GuiContainer(parent, caption) {
            
        static float x,y,z;
        static const float rotationControlWidth = 52;
        static const float captionWidth = 10;
        static const float rotationPrecision = 0.1f;
        static const float translationPrecision = 0.001f;
        static const std::string degrees = "\xba";
        static const float unitsSize = 8.0;
        GuiNumberBox<float>* c = NULL;
        static std::string s = "-100.00, -100.00, -100.00";

        m_centerBox = new GuiTextBox(this, "", &s, GuiTextBox::DELAYED_UPDATE, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE);
        m_centerBox->setSize(Vector2(160, CONTROL_HEIGHT));

        m_yawBox = new GuiNumberBox<float>(this, "", &x, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
        m_yawBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
        c = m_yawBox; c->setCaptionWidth(0);  c->setUnitsSize(unitsSize);

        m_pitchBox = new GuiNumberBox<float>(this, "", &y, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
        m_pitchBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
        c = m_pitchBox;  c->setCaptionWidth(0); c->setUnitsSize(unitsSize);

        m_rollBox = new GuiNumberBox<float>(this, "", &z, degrees, GuiTheme::NO_SLIDER, -finf(), finf(), rotationPrecision, GuiTheme::NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE); 
        m_rollBox->setSize(Vector2(rotationControlWidth, CONTROL_HEIGHT));
        c = m_rollBox; c->setCaptionWidth(0); c->setUnitsSize(unitsSize);

        // Change to black "r" (x) for remove
        const char* DOWN = "6";
        const char* CHECK = "\x98";
        const char* CLIPBOARD = "\xA4";
        GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
        GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

        m_bookmarkButton = new GuiButton(this, GuiButton::Callback(),GuiText(CHECK, iconFont, 16, Color3::blue() * 0.8f), 
//                GuiControl::Callback(this, &CameraControlWindow::onBookmarkButton),
                GuiTheme::TOOL_BUTTON_STYLE);

        m_showBookmarksButton = new GuiButton(this, GuiButton::Callback(), GuiText(DOWN, iconFont, 18), GuiTheme::TOOL_BUTTON_STYLE);

        m_copyButton = new GuiButton(this, GuiButton::Callback(), GuiText(CLIPBOARD, iconFont, 16), GuiTheme::TOOL_BUTTON_STYLE);//, GuiControl::Callback(this, &CameraControlWindow::copyToClipboard), GuiTheme::TOOL_BUTTON_STYLE);
#       ifdef G3D_OSX
            m_copyButton->setEnabled(false);
#       endif

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
    
    ~GuiCFrameBox() {
        for (int c = 0; c < NUM_CHILDREN; ++c) {
            delete m_child[c];
        }
    }

    virtual void setCaption(const std::string& c) {
        GuiContainer::setCaption(c);

        // Resize other parts in response to caption size changing
        setRect(m_rect);
    }
    
    virtual void setEnabled(bool e) {
        for (int c = 0; c < NUM_CHILDREN; ++c) {
            m_child[c]->setEnabled(e);
        }
    }

    virtual void setRect(const Rect2D& rect) {
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

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override {
        if (! m_clientRect.contains(mouse) || ! m_visible || ! m_enabled) {
            return;
        }

        mouse -= m_clientRect.x0y0();
        for (int c = 0; c < NUM_CHILDREN; ++c) {
            m_child[c]->findControlUnderMouse(mouse, control);
        }
    }


    virtual void render(RenderDevice* rd, const GuiThemeRef& skin) const override {
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
};


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

    debugPane->addCustom(new GuiCFrameBox(debugPane, "CFrame"));

#if 0
    std::string materialPath = System::findDataFile("material");
    std::string crateFile = System::findDataFile("crate.ifs");
    model = ArticulatedModel::fromFile(crateFile);
    Material::Specification mat;
    std::string base = pathConcat(materialPath, "metalcrate/metalcrate-");
    mat.setLambertian(base + "L.png", 0.2f);
    mat.setSpecular(base + "G.png");
    mat.setGlossyExponentShininess(20);
    BumpMap::Settings b;
    b.iterations = 1;
    mat.setBump(base + "B.png", b);
    Material::Ref material = Material::create(mat);
    /*

    // Save material
    {
        BinaryOutput b("material.mat.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        material->speedSerialize(sid, b);
        b.commit();
    }

    // Load material
    {
        BinaryInput b("material.mat.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        material = Material::speedCreate(sid, b);
    }*/

    model->partArray[0].triList[0]->material = material;
#endif

#if 0 // sponza
    Stopwatch timer;
    ArticulatedModel::Ref model = ArticulatedModel::fromFile(System::findDataFile("crytek_sponza/sponza.obj"));
    timer.after("Load OBJ");
    // Save Model
    { 
        BinaryOutput b("model.am.sl", G3D_LITTLE_ENDIAN);
        model->speedSerialize(b);
        b.commit();
    }
    timer.after("speedSerialize");

    // Load Model
    {
        BinaryInput b("model.am.sl", G3D_LITTLE_ENDIAN);
        SpeedLoadIdentifier sid;
        model = ArticulatedModel::speedCreate(b);
    }
    timer.after("speedDeserialize");
#endif

    lighting = defaultLighting();
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

    switch (e.type) {
    case GEventType::KEY_DOWN:
        debugPrintf("KEY_DOWN: %d (LSHIFT = %d)\n", e.key.keysym.sym, GKey::LSHIFT);
        break;

    case GEventType::KEY_UP:
        debugPrintf("KEY_UP: %d\n", e.key.keysym.sym);
        break;
    }
    
    return false;
}

void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    screenPrintf("LShift down: %d\n", userInput->keyDown(GKey::LSHIFT));
    screenPrintf("RShift down: %d\n", userInput->keyDown(GKey::RSHIFT));
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);

//    model->pose(surface3D);
    Surface::sortAndRender(rd, defaultCamera, surface3D, lighting);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
