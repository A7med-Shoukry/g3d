#ifndef GLG3D_ArticulatedModel2_h
#define GLG3D_ArticulatedModel2_h

#include <G3D/G3DAll.h>


/**
 \brief A 3D object composed of multiple rigid triangle meshes connected by joints.

 Supports OBJ file format.

 \sa ArticulatedModel

 \beta This is a candidate to replace G3D::ArticulatedModel in G3D 9.00
*/
class ArticulatedModel2 : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<ArticulatedModel2> Ref;

    /** Parameters for  cleanGeometry() */
    class CleanGeometrySettings {
    public:
        
        /** Set to true to check for redundant vertices even if 
           no normals or tangents need to be computed. This may increase
           rendering performance and decrease cleanGeometry() performance.           
           Default: true.
           */
        bool                        forceVertexMerging;

        /**
          Maximum angle in radians that a normal can be bent through to merge two vertices. 
          Default: 5 degrees().
          */
        float                       maxNormalWeldAngle;

        /** 
        Maximum angle in radians between the normals of adjacent faces that will still create
        the appearance of a smooth surface between them.  Alternatively, the minimum
        angle between those normals required to create a sharp crease.

        Set to 0 to force faceting of a model.  Set to 2 * pif() to make completely smooth.

        Default: 45 degrees().
        */
        float                       maxSmoothAngle;

        CleanGeometrySettings() : 
            forceVertexMerging(true), 
            maxNormalWeldAngle(5 * units::degrees()),
            maxSmoothAngle(45 * units::degrees()) {
        }
    };

    /** \brief Parameters for constructing a new ArticulatedModel from a file on disk.*/
    class Specification {
    public:
        /** Materials will be loaded relative to this file.*/
        std::string                 filename;

        /** Transformation to apply to vertices in the global reference frame at load time.
        The inverse transpose of the upper 3x3 is applied to normals.
        */
        Matrix4                     xform;

        /** Ignore materials specified in the file, replacing them with Material::create().
        Setting to true increases loading performance and may allow more aggressive 
        optimization if mergeMeshesByMaterial is also true.
        */
        bool                        stripMaterials;

        /** Merge all Mesh%es within a Part that share a material into a single Mesh. 
        This will increase the rendering performance of objects that were divided 
        into many parts by an artist for modeling purposes.  It may decrease the 
        rendering performance of very large objects that were divided into pieces
        that are unlikely to all be visible in the frustum simultaneously. 
        */
        bool                        mergeMeshesByMaterial;

        CleanGeometrySettings       cleanGeometrySettings;
    };

    class Mesh {
    public:
        friend class ArticulatedModel2;

        std::string                 name;

        Material::Ref               material;

        PrimitiveType               primitive;
        Array<int>                  cpuIndexArray;
        VertexRange                 gpuIndexArray;

        bool                        twoSided;

        /** Relative to the Part containing it. */
        Sphere                      boundingSphere;
    private:
        Mesh(const std::string& name) : name(name), primitive(PrimitiveType::TRIANGLES), twoSided(false) {}

    };

    /** 
     A set of meshes with a single reference frame, packed into a common vertex buffer.
    */
    class Part {
    public:
        friend class ArticulatedModel2;

        /** Packed vertex attributes */
        class Vertex {
        public:
            /** Part-space position */
            Point3                  position;

            /** Part-space normal */
            Vector3                 normal;

            /** Texture coordinate 0, setting a convention for expansion in later API versions. */
            Point2                  texCoord0;

            /** xyz = tangent, w = sign */
            Vector4                 tangent;
        };

    private:
       
        /** Used by cleanGeometry */
        class Face {
        public:

            /** Index of a Face in a temporary array*/
            typedef int Index;
            typedef SmallArray<Index, 7> IndexArray;
            typedef Table<Point3, IndexArray> AdjacentFaceTable;

            Vertex      vertex[3];

            /** Mesh from which this face was originally created. Needed for reconstructing
              the index arrays after vertices are merged.*/
            Mesh*       mesh;
            
            /** Non-unit face normal, used for weighted vertex normal computation */
            Vector3     normal;
            Vector3     unitNormal;

            Face() : mesh(NULL) {}
        };

    public:

        std::string                 name;

    private:

        bool                        m_hasTexCoord0;
        Part*                       m_parent;
        Array<Part*>                m_child;
        Array<Mesh*>                m_meshArray;
        int                         m_triangleCount;

    public:

        /** Transformation from this object to the parent's frame in the rest pose */
        CFrame                      cframe;
        Sphere                      boundingSphere;

        Array<Vertex>               cpuVertexArray;

        VertexRange                 gpuPositionArray;
        VertexRange                 gpuNormalArray;
        VertexRange                 gpuTexCoord0Array;
        VertexRange                 gpuTangentArray;

    private:

        /** Discards all gpu vertex range data */
        void clearVertexRanges();

        /** Called from cleanGeometry to determine what needs to be computed. */
        void determineCleaningNeeds(bool& computeSomeNormals, bool& computeSomeTangents);

        /** Called from cleanGeometry */
        void buildFaceArray(Array<Face>& faceArray, Face::AdjacentFaceTable& adjacentFaceTable);

        /** Called from cleanGeometry.  Computes all vertex normals that are currently NaN. */
        void computeMissingVertexNormals
         (Array<Face>&                      faceArray, 
          const Face::AdjacentFaceTable&    adjacentFaceTable, 
          const float                       maximumSmoothAngle);

        /** Called from cleanGeometry.  Collapses shared vertices back into cpuVertexArray.
        \param maxNormalWeldAngle Maximum amount a normal can be bent to merge two vertices. */
        void mergeVertices(const Array<Face>& faceArray, float maxNormalWeldAngle);

        /** Called from cleanGeometry(). Computes all tangents that are currently NaN.*/
        void computeMissingTangents();

        Part(const std::string& name, Part* parent) : name(name), m_parent(parent) {}

    public:

        /** NULL if this is a root of the model. */
        const Part* parent() const {
            return m_parent;
        }

        /** Computed by cleanGeometry() */
        int triangleCount() const {
            return m_triangleCount;
        }

        const Array<Part*>& childArray() const {
            return m_child;
        }

        const Array<Mesh*>& meshArray() const {
            return m_meshArray;
        }

        /** True if texCoord0 is not all zero. */
        bool hasTexCoord0() const {
            return m_hasTexCoord0;
        }

        bool isRoot() const {
            return m_parent == NULL;
        }

        /** 
         Cleans the geometric data in response to changes, or after load.

        - Wipes out the GPU vertex attribute data.
        - Computes a vertex normal for every element whose normal.x is fnan() (or if the normal array is empty).
        - If there are texture coordiantes, computes a tangent for every element whose tangent.x is nanf() (or if the tangent array is empty).
        - Merges all vertices with identical indices.
        - Updates all Mesh indices accordingly.
        - Recomputes the bounding sphere.

        */
        void cleanGeometry(const CleanGeometrySettings& settings);
    };

    /** Base class for defining operations to perform on each part, in hierarchy order.*/
    class PartCallback {
    public:
        virtual void operator()(ArticulatedModel2::Ref m, ArticulatedModel2::Part* p, const CFrame& parentFrame) {}
    };

private:

    Array<Part*>                    m_rootArray;
    Array<Part*>                    m_partArray;

private:

    void forEachPart(PartCallback& c, const CFrame& parentFrame, Part* part);


public:

    /** \sa createEmpty */
    static Ref create(const Specification& s);

    /** \sa create, addMesh, addPart */
    static Ref createEmpty(const std::string& name);

    /** Root parts.  There may be more than one. */
    const Array<Part*>& rootArray() const;

    /** \sa addPart, createEmpty */
    Mesh* addMesh(const std::string& name, Part* part) {
        part->m_meshArray.append(new Mesh(name));
        return part->m_meshArray.last();
    }

    /** \sa addMesh, createEmpty */
    Part* addPart(const std::string& name, Part* parent = NULL) {
        m_partArray.append(new Part(name, parent));
        if (parent == NULL) {
            m_rootArray.append(m_partArray.last());
        }
        return m_partArray.last();
    }

    /** Walks the hierarchy and invokes PartCallback \a c on each Part. */
    void forEachPart(PartCallback& c);
    
    /** 
      Invokes Part::cleanGeometry on all parts.

        \param alwaysMergeVertices  Set to true to check for redundant vertices even if 
          no normals or tangents need to be computed.
     */
    void cleanGeometry(const CleanGeometrySettings& settings);

private:

    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);

    void load(const Specification& specification);
};

#endif
