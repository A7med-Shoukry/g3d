/**
 \file GLG3D/ParseOBJ.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2011-07-19
 \edited  2011-07-19

 Copyright 2002-2011, Morgan McGuire.
 All rights reserved.
*/

#ifndef GLG3D_ParseOBJ_h
#define GLG3D_ParseOBJ_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/SmallArray.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"
#include "G3D/ParseMTL.h"

namespace G3D {

class TextInput;

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
        but is unlimited in OBJ format. */
    typedef SmallArray<Index, 4> Face;
    
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

public:

    void parse(TextInput& ti, const std::string& basePath = "<AUTO>");

};


template <> struct HashTrait<ParseOBJ::Index> {
    static size_t hashCode(const ParseOBJ::Index& k) { 
        return HashTrait<int>::hashCode(k.vertex + (k.normal << 8) + (k.texCoord << 16) + (k.texCoord >> 16)); 
    }
};


} // namespace G3D
#endif // GLG3D_ParseOBJ_h
