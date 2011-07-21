#ifndef GLG3D_ParseOBJ_h
#define GLG3D_ParseOBJ_h

#include <G3D/G3DAll.h>
#include "ParseMTL.h"

/** \brief Parses OBJ files with polygonal data and their associated MTL files.

Ignores groups, smoothing groups, surfaces, object names. Assumes
that each face is in exactly one group.  Note that group information may be useful
for object-level culling.

\cite http://www.martinreddy.net/gfx/3d/OBJ.spec

\sa G3D::ParseMTL, G3D::ArticulatedModel
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

    private:

        Mesh() {}

    public:

        static Ref create() {
            return new Mesh();
        }
    };

    /** An OBJ group, created with the "g" command. */
    class Group : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Group> Ref;

        std::string     name;

        /** Maps material names to meshes within this group */
        Table<std::string, Mesh::Ref> meshTable;

    private:

        Group() {}

    public:

        static Ref create() {
            return new Group();
        }    
    };

    Array<Point3>       vertexArray;
    Array<Vector3>      normalArray;
    Array<Point2>       texCoordArray;

    /** Maps group names to groups. */
    Table<std::string, Group::Ref> groupTable;

private:

    /** The material table can be replaced during load, although rarely is. */
    ParseMTL            m_currentMaterialTable;

    /** Paths are interpreted relative to this */
    std::string         m_basePath;

    /** Group to which we are currently adding elements */
    Group::Ref          m_currentGroup;

    /** Mesh within m_currentGroup to which we are currently adding elements */
    Mesh::Ref           m_currentMesh;

    void processCommand(TextInput& ti, const std::string& cmd);

public:

    void parse(TextInput& ti, const std::string& basePath = "<AUTO>");

};

#endif
