#include "ArticulatedModel2.h"

/** Parses OBJ files with polygonal data and their associated MTL files.

Ignores groups, smoothing groups, surfaces, object names. Assumes
that each face is in exactly one group.  Note that group information may be useful
for object-level culling.

\cite http://www.martinreddy.net/gfx/3d/OBJ.spec
*/
class ParseOBJ {
public:
    static const int UNDEFINED = -1;

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

    typedef SmallArray<Index, 4> Face;
    
    /** Part of a group that uses a single material. */
    class Mesh : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Mesh> Ref;

        Material::Ref   material;
        Array<Face>     faceArray;
    };

    /** An OBJ group, created with the "g" command. */
    class Group : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Group> Ref;

        std::string     name;

        /** Maps material names to meshes within this group */
        Table<std::string, Mesh::Ref> meshTable;
    };

    Array<Point3>       vertexArray;
    Array<Vector3>      normalArray;
    Array<Point2>       texCoordArray;

    /** Maps group names to groups. */
    Table<std::string, Group::Ref> groupTable;

private:

    /** The material table can be replaced during load, although rarely is. */
    ParseMTL            m_currentMaterialTable;

    /** Group to which we are currently adding elements */
    Group::Ref          m_currentGroup;

    /** Mesh within m_currentGroup to which we are currently adding elements */
    Mesh::Ref           m_currentMesh;

public:

    void parse(TextInput& ti);

};


void ArticulatedModel2::loadOBJ(const Specification& specification) {
    ParseOBJ parser;
    parser.parse(TextInput(specification.filename));
}





/** \brief Parses Wavefront material (.mtl) files.

     \sa G3D::ParseOBJ
*/
class ParseMTL {
public:

    /** Loaded from the MTL file */
    class Material : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Material> Ref;
        std::string     name;

        /** Ambient color of the material, on the range 0-1 */
        Color3          Ka;
        std::string     map_Ka;

        /** Diffuse color of the material, on the range 0-1 */
        Color3          Kd;
        std::string     map_Kd;

        /** Specular color of the material, on the range 0-1. */
        Color3          Ks;
        std::string     map_Ks;

        /** Shininess of the material, on the range 0-1000. */
        float           Ns;

        /** map_bump/bump field filename*/
        std::string     map_bump;

        /** Opacity/alpha level, on the range 0-1 */
        float           d;

        /** Transparency level, on the range 0-1. Amount of light transmitted.*/
        float           Tr;

        /** Unknown */
        Color3          Tf;

        /** emissive? */
        Color3          Ke;

        /** Illumination model enumeration on the range 0-10. */
        int             illum;

        /** Index of refraction */
        float           Ni;

        Material() : Ka(1.0f), Kd(1.0f), Ks(1.0f), Ns(10.0), d(1.0f), Tr(0.0f), Tf(1.0f), Ke(0.0f), illum(2), Ni(1.5f) {}
    };
    
    Table<std::string, Material::Ref> materialTable;

private:
    /** Process one line of an OBJ file */
    void processCommand(TextInput& ti, const std::string& cmd);

    Material::Ref       m_currentMaterial;

    std::string         m_basePath;

public:

    /** \param basePath Directory relative to which texture filenames are resolved. If "<AUTO>", the 
     path to the TextInput%'s file is used. */
    void parse(TextInput& ti, const std::string& basePath = "<AUTO>");

};



void ParseMTL::parse(TextInput& ti, const std::string& basePath) {
    materialTable.clear();

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;
    ti.pushSettings(set);

    m_basePath = basePath;
    if (m_basePath == "<AUTO>") {
        m_basePath = FilePath::parent(FileSystem::resolve(ti.filename()));
    }

    while (ti.hasMore()) {
        // Consume comments/newlines
        while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
            // Consume the newline
            ti.read();
        }

        // Process one line
        const std::string& cmd = ti.readSymbol();

        processCommand(ti, cmd);

        // Read until the end of the line if this line did not consume it
        while (ti.hasMore() && (ti.read().type() != Token::NEWLINE));
    }

    ti.popSettings();
}


void ParseMTL::processCommand(TextInput& ti, const std::string& cmd) {

    if (cmd == "newmtl") {
        // Create a new material
        m_currentMaterial = new Material();
        m_currentMaterial->name = trimWhitespace(ti.readUntilNewlineAsString());
        materialTable.set(m_currentMaterial->name, m_currentMaterial);

    } else if (cmd == "d") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            // "dissolve"; alpha on range [0,1]
            if (ti.peek().type() == Token::SYMBOL) {
                // Optional "-halo" 
                ti.readSymbol();
            }
            m_currentMaterial->opacity = ti.readNumber();
        }
    } else if (cmd == "Tr") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            // 1 - alpha on range [0,1]
            m_currentMaterial->Tr = ti.readNumber();
        }
    } else if (cmd == "Ns") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            // 1 - alpha on range [0,1]
            m_currentMaterial->Ns = ti.readNumber();
        }
    } else if (cmd == "Ni") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            // 1 - alpha on range [0,1]
            m_currentMaterial->Ni = ti.readNumber();
        }
    } else if (cmd == "Ka") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            m_currentMaterial->Ka.r = ti.readNumber();
            m_currentMaterial->Ka.g = ti.readNumber();
            m_currentMaterial->Ka.b = ti.readNumber();
        }
    } else if (cmd == "Ka") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            m_currentMaterial->Kd.r = ti.readNumber();
            m_currentMaterial->Kd.g = ti.readNumber();
            m_currentMaterial->Kd.b = ti.readNumber();
        }
    } else if (cmd == "Ks") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            m_currentMaterial->Ks.r = ti.readNumber();
            m_currentMaterial->Ks.g = ti.readNumber();
            m_currentMaterial->Ks.b = ti.readNumber();
        }
    } else if (cmd == "Ke") {
        if (m_currentMaterial.isNull()) {
            logPrintf("Warning: encountered command with null material\n");
        } else {
            m_currentMaterial->Ke.r = ti.readNumber();
            m_currentMaterial->Ke.g = ti.readNumber();
            m_currentMaterial->Ke.b = ti.readNumber();
        }
    } else if (cmd == "map_Kd") {
        matSpec.diffuseMap = FilePath::concat(basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
        if (! FileSystem::exists(matSpec.diffuseMap)) {
            debugPrintf("OBJ WARNING: Missing diffuse texture map '%s'\n", matSpec.diffuseMap.c_str());
            matSpec.diffuseMap = "";
        }
    } else if (cmd == "map_Ks") {
        matSpec.glossyMap = FilePath::concat(basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
        if (! FileSystem::exists(matSpec.glossyMap)) {
            debugPrintf("OBJ WARNING: Missing glossy texture map '%s'\n", matSpec.glossyMap.c_str());
            matSpec.glossyMap = "";
        }
    } else if (cmd == "map_bump" || cmd == "bump") {
        Token t = ti.peek();
        if (t.type() == Token::SYMBOL && t.string() == "-") {
            // There are options coming
            ti.readSymbol("-");
            const std::string& opt = ti.readSymbol();
            if (opt == "mm") {
                // bias and gain
                matSpec.bumpBias = ti.readNumber();
                matSpec.bumpGain = ti.readNumber();
            }
        }
        matSpec.bumpMap = FilePath::concat(basePath, removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString())));
        if (! FileSystem::exists(matSpec.bumpMap)) {
            debugPrintf("OBJ WARNING: Missing bump map '%s'\n", matSpec.bumpMap.c_str());
            matSpec.bumpMap = "";
        }
    }
}
