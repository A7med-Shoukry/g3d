/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();


/** High-performance OBJ parser */
class ParseOBJ2 {
private:

    /** Pointer to the next character in buffer */
    char*   nextCharacter;

    /** Number of characters left */
    int     remainingCharacters;

    /** Line in the file, starting from 1.  For debugging and error reporting */
    int     line;

    /** Consume one character */
    inline void consumeCharacter() {
        ++nextCharacter;
        --remainingCharacters;
    }


    /** Reads until the end of file or newline, but does not consume the newline */
    void readUntilNewline() {
        while ((remainingCharacters > 0) && (*nextCharacter != '\r') && (*nextCharacter != '\n')) {
            consumeCharacter();
        }
    }


    /** Consume whitespace and comments, if there are any.  Leaves the pointer on the first non-whitespace character. */
    void maybeReadWhitespace() {
        while (remainingCharacters > 0) {
            switch (*nextCharacter) {
            case '\n':
            case '\r':
                {
                    char c = *nextCharacter;
                    consumeCharacter();
                    ++line;
                    if ((remainingCharacters > 0) && (c != *nextCharacter) && (*nextCharacter != '\r') && (*nextCharacter != '\n')) {
                        // This is part of a double-newline, e.g., Mac or Windows.  Consume the next character as well.
                        consumeCharacter();
                    }
                }
                break;

            case ' ':
            case '\t':
                // Consume whitespace
                consumeCharacter();
                break;

            case '#':
                // Consume comment
                readUntilNewline();
                // Don't consume the newline; we'll catch it on the next iteration
                break;

            default:
                return;
            }
        }
    }

    enum Command {MTLLIB, GROUP, USEMTL, VERTEX, TEXCOORD, NORMAL, FACE, UNKNOWN};

    static bool isSpace(const char c) const {
        return c == ' ' || c == '\t';
    }

    /** Reads the next command.  Assumes that it is at the start of a command. */
    Command readCommand() {
        if (remainingCharacters == 0) {
            return UNKNOWN;
        }

        switch (*nextCharacter) {
        case 'f':
            consumeCharacter();
            if (isSpace(*nextCharacter)) {
                return FACE;
            } else {
                return UNKNOWN;
            }
            break;

        case 'v':
            consumeCharacter();
            switch (*nextCharacter) {
            case ' ':
            case '\t':
                return VERTEX;

            case 'n':
                consumeCharacter();
                if (isSpace(*nextCharacter)) {
                    return NORMAL;
                } else {
                    return UNKNOWN;
                }
                break;

            case 't':
                consumeCharacter();
                if (isSpace(*nextCharacter)) {
                    return TEXCOORD;
                } else {
                    return UNKNOWN;
                }
                break;

            default:
                return UNKNOWN;
            }
            break;

        case 'm':
            if ((remainingCharacters > 6) && memcmp(nextCharacter, "mtllib", 6)) {
                nextCharacter += 6; remainingCharacters -= 6;
                if (isSpace(*nextCharacter)) {
                    return MTLLIB;
                } else {
                    return UNKNOWN;
                }
            }
            break;

        case 'u':
            if ((remainingCharacters > 6) && memcmp(nextCharacter, "usemtl", 6)) {
                nextCharacter += 6; remainingCharacters -= 6;
                if (isSpace(*nextCharacter)) {
                    return USEMTL;
                } else {
                    return UNKNOWN;
                }
            }
            break;

        case 'g':
            consumeCharacter();
            if (isSpace(*nextCharacter)) {
                return GROUP;
            } else {
                return UNKNOWN;
            }
            break;


        default:
            return UNKNOWN;
        }
    }


    void processCommand(const Command command) {
        switch (command) {
        case VERTEX:
            // TODO
            break;

        case TEXCOORD:
            // TODO
            break;

        case NORMAL:
            // TODO
            break;

        case FACE:
            // TODO
            break;

        case GROUP:
            // TODO
            break;

        case USEMTL:
            // TODO
            break;

        case MTLLIB:
            // TODO
            break;

        case UNKNOWN:
            // Nothing to do
            break;
        }
    }

public:

    void parse(char* ptr, int len, const std::string& basePath) {
        nextCharacter = ptr;
        remainingCharacters = len;
        line = 1;

        while (remainingCharacters > 0) {
            // Process leading whitespace
            maybeReadWhitespace();

            const Command command = readCommand();
            processCommand(command);

            // Consume anything else on this line
            readUntilNewline();
        }
        
    }
};


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

    return false;
}

void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);

    model->pose(surface3D);
    Surface::sortAndRender(rd, defaultCamera, surface3D, lighting);

    // Call to make the GApp show the output of debugDraw
    drawDebugShapes();
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
