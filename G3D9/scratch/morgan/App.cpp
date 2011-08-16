/** \file App.cpp */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();



/** \brief Parses OBJ files with polygonal data and their associated MTL files.

Ignores groups, smoothing groups, surfaces, object names. Assumes
that each face is in exactly one group.  Note that group information may be useful
for object-level culling.

\cite http://www.martinreddy.net/gfx/3d/OBJ.spec

Uses a special text parser instead of G3D::TextInput for peak performance (about 30x faster
than TextInput).

\sa G3D::ParseMTL, G3D::ParsePLY, G3D::Parse3DS, G3D::ArticulatedModel2
*/
class ParseOBJ2 {
public:
    static const int UNDEFINED = -1;

    /** Set of indices into the vertex attribute arrays.  Note that OBJ
        format allows a separate index for each attribute, unlike OpenGL. */
    class Index {
    public:
        /** 0-based index into vertexArray */
        int             vertex;

        /** 0-based index into normalArray */
        int             normal;

        /** 0-based index into texCoordArray */
        int             texCoord;

        Index() : vertex(UNDEFINED), normal(UNDEFINED), texCoord(UNDEFINED) {}
    };

    /** A polygon, which is expected to be a triangle or quadrilateral 
        but is unlimited in OBJ format.         
        */
    // We reserve 5 sides before going to heap allocation because
    // that was observed to save 1/2 second when loading Crytek Sponza.
    typedef SmallArray<Index, 5> Face;
    
    /** Part of a group that uses a single material. */
    class Mesh : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Mesh> Ref;

        /** Need a material instead of a material name
            because technically the material library can
            change during load.
         */
        ParseMTL::Material::Ref   material;
        Array<Face>     faceArray;

    private:

        Mesh() {}

    public:

        static Ref create() {
            return new Mesh();
        }
    };

    typedef Table<ParseMTL::Material::Ref, Mesh::Ref> MeshTable;

    /** An OBJ group, created with the "g" command. 
    */
    class Group : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Group> Ref;

        std::string     name;

        /** Maps ParseMTL::Material%s to Mesh%ss within this group */
        MeshTable       meshTable;

    private:

        Group() {}

    public:

        static Ref create() {
            return new Group();
        }    
    };


    typedef Table<std::string, Group::Ref> GroupTable;

    Array<Point3>       vertexArray;
    Array<Vector3>      normalArray;

    /** Texture coordinates in OBJ coordinate frame, where (0, 0) is the LOWER-left.*/
    Array<Point2>       texCoordArray;

    /** Maps group names to groups. */
    GroupTable          groupTable;

private:

    std::string         m_filename;

    /** Pointer to the next character in buffer */
    const char*         nextCharacter;

    /** Number of characters left */
    int                 remainingCharacters;

    /** Line in the file, starting from 1.  For debugging and error reporting */
    int                 m_line;


    /** The material library can be replaced during load, although rarely is. */
    ParseMTL            m_currentMaterialLibrary;

    /** Paths are interpreted relative to this */
    std::string         m_basePath;

    /** Group to which we are currently adding elements */
    Group::Ref          m_currentGroup;

    /** Mesh within m_currentGroup to which we are currently adding elements. 
      Determined by the material name. */
    Mesh::Ref           m_currentMesh;

    /** Material specified by the last useMtl command */
    ParseMTL::Material::Ref  m_currentMaterial;

    void processCommand(TextInput& ti, const std::string& cmd);

    /** Processes the "f" command.  Called from processCommand. */
    void processFace(TextInput& ti);

    ParseMTL::Material::Ref getMaterial(const std::string& materialName);
    
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

    void readFace();

    /** Consume whitespace and comments, if there are any.  Leaves the pointer on the first non-whitespace character. 
    Returns true if an end-of-line was passed or the end of file was reached. */
    bool maybeReadWhitespace();

    enum Command {MTLLIB, GROUP, USEMTL, VERTEX, TEXCOORD, NORMAL, FACE, UNKNOWN};

    /** Returns true for space and tab, but not newline */
    static inline bool isSpace(const char c) {
        return (c == ' ') || (c == '\t');
    }

    /** Reads the next command.  Assumes that it is at the start of a command. 
      Leaves the pointer at the first character after the end of the command name.*/
    Command readCommand();

    template <class T>
    T readNumber(const char* fmt) {
        // Scan for the end of the token
        char old = '\0';
        int i = 0;
        while  (i < remainingCharacters) {
            switch (nextCharacter[i]) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '/':
            case '#':
                old = nextCharacter[i];
                // Overwrite with string terminator to stop sscanf
                const_cast<char&>(nextCharacter[i]) = '\0';
                break;

            default:;
                ++i;
                // Continue on
            }
        }

        T n = 0;
        const int numRead = sscanf(nextCharacter, fmt, &n);

        if (numRead == 0) {
            // Something went wrong
            throw ParseError(m_filename, m_line, 0, "Expected float on this line");
        }

        if (old != '\0') {
            // Restore the old character
            const_cast<char&>(nextCharacter[i]) = old;
        }

        // Jump to character i
        nextCharacter += i;
        remainingCharacters -= i;

        return n;    
    }

    inline float readFloat() {
        return readNumber<float>("%f");
    }

    inline int readInt() {
        return readNumber<int>("%d");
    }

    /** Reads until newline and removes leading and trailing space (ignores comments) */
    std::string readName() {
        // TODO
        alwaysAssertM(false, "TODO");
        return "";
    }

    Vector3 readVector3() {
        Vector3 v;
        v.x = readFloat();
        v.y = readFloat();
        v.z = readFloat();
        return v;
    }

    Vector2 readVector2() {
        Vector2 v;
        v.x = readFloat();
        v.y = readFloat();
        return v;
    }

    void processCommand(const Command command);

public:

    void parse(const char* ptr, int len, const std::string& basePath);

    void parse(BinaryInput& bi, const std::string& basePath = "<AUTO>");
};





template <> struct HashTrait<ParseOBJ2::Index> {
    static size_t hashCode(const ParseOBJ2::Index& k) { 
        return HashTrait<int>::hashCode(k.vertex + (k.normal << 8) + (k.texCoord << 16) + (k.texCoord >> 16)); 
    }
};



void ParseOBJ2::parse(const char* ptr, int len, const std::string& basePath) {
    vertexArray.clear();
    normalArray.clear();
    texCoordArray.clear();
    groupTable.clear();
    m_basePath = "";
    m_currentGroup = NULL;
    m_currentMesh = NULL;
    m_currentMaterial = NULL;

    m_basePath = basePath;

    nextCharacter = ptr;
    remainingCharacters = len;
    m_line = 1;

    while (remainingCharacters > 0) {
        // Process leading whitespace
        maybeReadWhitespace();

        const Command command = readCommand();
        processCommand(command);

        // Consume anything else on this line
        readUntilNewline();
    }        
}

void ParseOBJ2::parse(BinaryInput& bi, const std::string& basePath) {
    m_filename = bi.getFilename();

    std::string bp = basePath;
    if (bp == "<AUTO>") {
        bp = FilePath::parent(FileSystem::resolve(m_filename));
    }
    
    

    parse((const char*)bi.getCArray() + bi.getPosition(), bi.getLength() - bi.getPosition(), bp);
}


ParseMTL::Material::Ref ParseOBJ2::getMaterial(const std::string& materialName) {
    bool created = false;
    ParseMTL::Material::Ref& m = m_currentMaterialLibrary.materialTable.getCreate(materialName);
    if (created) {
        m = ParseMTL::Material::create();
        debugPrintf("Warning: missing material %s used.\n", materialName.c_str());
    }
    return m;
}


bool ParseOBJ2::maybeReadWhitespace() {
    bool changedLines = false;

    while (remainingCharacters > 0) {
        switch (*nextCharacter) {
        case '\n':
        case '\r':
            {
                char c = *nextCharacter;
                consumeCharacter();
                ++m_line;
                changedLines = true;
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
            return changedLines;
        }
    }

    return true;
}


ParseOBJ2::Command ParseOBJ2::readCommand() {
    if (remainingCharacters == 0) {
        return UNKNOWN;
    }

    // Explicit finite automata parser
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
        } else {
            return UNKNOWN;
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
        } else {
            return UNKNOWN;
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


void ParseOBJ2::readFace() {
    // Ensure that we have a material
    if (m_currentMaterial.isNull()) {
        m_currentMaterial = m_currentMaterialLibrary.materialTable["default"];
    }

    // Mnsure that we have a group
    if (m_currentGroup.isNull()) {
        // Create a group named "default", per the OBJ specification
        m_currentGroup = Group::create();
        m_currentGroup->name = "default";
        groupTable.set(m_currentGroup->name, m_currentGroup);

        // We can't have a mesh without a group, but conservatively reset this anyway
        m_currentMesh = NULL;
    }

    // Ensure that we have a mesh
    if (m_currentMesh.isNull()) {
        bool created = false;
        Mesh::Ref& m = m_currentGroup->meshTable.getCreate(m_currentMaterial, created);

        if (created) {
            m = Mesh::create();
            m->material = m_currentMaterial;
        }
        m_currentMesh = m;
    }

    Face& face = m_currentMesh->faceArray.next();

    const int vertexArraySize   = vertexArray.size();
    const int texCoordArraySize = texCoordArray.size();
    const int normalArraySize   = normalArray.size();

    bool done = false;
    while (! done) {
        Index& index = face.next();

        // Read index
        index.vertex = readInt();
        if (index.vertex > 0) {
            // Make 0-based
            --index.vertex;
        } else {
            // Negative; make relative to the current end of the array.
            // -1 will be the last element, so just add the size of the array.
            index.vertex += vertexArraySize;
        }

        if ((remainingCharacters > 0) && (*nextCharacter == '/')) {
            consumeCharacter();
            if (remainingCharacters > 0) {
                if (*nextCharacter == '/') {
                    // No texcoord index
                    consumeCharacter();
                } else {
                    // texcoord index
                    index.texCoord = readInt();
                    if (index.texCoord > 0) {
                        // Make 0-based
                        --index.texCoord;
                    } else {
                        // Negative; make relative to the current end of the array.
                        // -1 will be the last element, so just add the size of the array.
                        index.texCoord += texCoordArraySize;
                    }  
                }

                if ((remainingCharacters > 0) && (*nextCharacter == '/')) {
                    if (*nextCharacter == '/') {
                        // No normal index
                        consumeCharacter();
                    } else {
                        // normal index
                        index.normal = readInt();
                        if (index.normal > 0) {
                            // Make 0-based
                            --index.normal;
                        } else {
                            // Negative; make relative to the current end of the array.
                            // -1 will be the last element, so just add the size of the array.
                            index.normal += normalArraySize;
                        }       
                    }
                }
            }
        }

        // Read remaining whitespace
        done = maybeReadWhitespace();
    }
}

void ParseOBJ2::processCommand(const Command command) {
    switch (command) {
    case VERTEX:
        vertexArray.append(readVector3());
        break;

    case TEXCOORD:
        texCoordArray.append(readVector2());
        break;

    case NORMAL:
        normalArray.append(readVector3());
        break;

    case FACE:
        readFace();
        break;

    case GROUP:
        {
            // Change group
            std::string groupName = readName();

            Group::Ref& g = groupTable.getCreate(groupName);

            if (g.isNull()) {
                // Newly created
                g = Group::create();
                g->name = groupName;
            }

            m_currentGroup = g;
        }
        break;

    case USEMTL:
        {
            // Change the mesh within the group
            const std::string& materialName = readName();        
            m_currentMaterial = getMaterial(materialName);

            // Force re-obtaining or creating of the appropriate mesh
            m_currentMesh = NULL;
        }
        break;

    case MTLLIB:
        {
            // Specify material library 
            std::string mtlFilename = readName();
            mtlFilename = FilePath::concat(m_basePath, mtlFilename);

            TextInput ti2(mtlFilename);
            m_currentMaterialLibrary.parse(ti2);
        }
        break;

    case UNKNOWN:
        // Nothing to do
        break;
    }
}












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

    TextInput ti(System::findDataFile("models/dragon/dragon.obj.zip/dragon.obj"));
    BinaryInput bi(System::findDataFile("models/dragon/dragon.obj.zip/dragon.obj"), G3D_LITTLE_ENDIAN);

    Stopwatch s;
    if (false) {
        ParseOBJ oldParser;
        oldParser.parse(ti);
    }
    s.after("old");
    {
        ParseOBJ2 newParser;
        newParser.parse((const char*)bi.getCArray(), bi.getLength(), "");
    }
    s.after("new");
    ::exit(0);

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
