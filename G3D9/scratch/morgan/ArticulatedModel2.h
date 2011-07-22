#ifndef GLG3D_ArticulatedModel2_h
#define GLG3D_ArticulatedModel2_h

#include <G3D/G3DAll.h>


/**
 \brief A 3D object composed of multiple rigid triangle meshes connected by joints.

 Supports OBJ file format.

 \sa ArticulatedModel

 \beta This is a candidate to replace G3D::ArticulatedModel in G3D 9.00

 TODO:
 - create SuperSurfaces
 - transform
 - intersect
 - load other formats: IFS, PLY2, PLY, 3DS
 - create heightfield
 - create cornell box
 - Pack tangents into short4 format?
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

        explicit CleanGeometrySettings(const Any& a);
        
        CleanGeometrySettings& operator=(const Any& a) {
            *this = CleanGeometrySettings(a);
            return *this;
        }

        Any toAny() const;
    };

    /** \brief Parameters for constructing a new ArticulatedModel from a file on disk.*/
    class Specification {
    public:
        /** Materials will be loaded relative to this file.*/
        std::string                 filename;

        /** Transformation to apply to vertices in the global reference frame at load time.
        The inverse transpose of the upper 3x3 is applied to normals.

        Default: Matrix4::identity()
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

        Default: false
        */
        bool                        mergeMeshesByMaterial;

        CleanGeometrySettings       cleanGeometrySettings;

        Specification() : xform(Matrix4::identity()), stripMaterials(false), mergeMeshesByMaterial(false) {}
        explicit Specification(const Any& a);
        Any toAny() const;
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

        /** Relative to the Part containing it. */
        AABox                       boundingBox;

    private:

        Mesh(const std::string& name) : name(name), primitive(PrimitiveType::TRIANGLES), twoSided(false) {}

    };


    /** Specifies the transformation that occurs at each node in the heirarchy. 
     */
    class Pose {
    public:
        /** Mapping from names to coordinate frames (relative to parent).
            If a name is not present, its coordinate frame is assumed to
            be the identity.
         */
        Table<std::string, CoordinateFrame>     cframe;

        Pose() {}
    };


    class PoseSpline {
    public:
        typedef Table<std::string, PhysicsFrameSpline> SplineTable;
        SplineTable partSpline;

        PoseSpline();

        /**
         The Any must be a table mapping part names to PhysicsFrameSplines.
         Note that a single PhysicsFrame (or any equivalent of it) can serve as
         to create a PhysicsFrameSpline.  

         <pre>
            PoseSpline {
                "part1" = PhysicsFrameSpline {
                   control = ( Vector3(0,0,0),
                               CFrame::fromXYZYPRDegrees(0,1,0,35)),
                   cyclic = true
                },

                "part2" = Vector3(0,1,0)
            }
         </pre>
        */
        PoseSpline(const Any& any);
     
        /** Get the pose at time t, overriding values in \a pose that are specified by the spline. */
        void get(float t, ArticulatedModel2::Pose& pose);
    };


    /** 
     \brief A set of meshes with a single reference frame, packed into a common vertex buffer.
    */
    class Part {
    private:
        friend class ArticulatedModel2;
       
        /** Used by cleanGeometry */
        class Face {
        public:

            /** Index of a Face in a temporary array*/
            typedef int Index;
            typedef SmallArray<Index, 7> IndexArray;
            typedef Table<Point3, IndexArray> AdjacentFaceTable;

            CPUVertexArray::Vertex   vertex[3];

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

        /** Bounding sphere of just this Part's geometry, in object space. Does not include child parts.*/
        Sphere                      boundingSphere;

        /** Bounding box of just this Part's geometry, in object space. Does not include child parts.*/
        AABox                       boundingBox;

        CPUVertexArray              cpuVertexArray;

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

        /** Pose this part and all of its children */
        void pose
            (Array<Surface::Ref>&     surfaceArray,
             const CoordinateFrame&   parentFrame,
             const Pose&              pose,
             const CoordinateFrame&   prevParentFrame,
             const Pose&              prevPose);
    };


    /** Base class for defining operations to perform on each part, in hierarchy order.*/
    class PartCallback {
    public:
        virtual void operator()(ArticulatedModel2::Ref m, ArticulatedModel2::Part* p, const CFrame& parentFrame) {}
    };

    /** The rest pose.*/
    static const Pose& defaultPose();

private:

    Array<Part*>                    m_rootArray;
    Array<Part*>                    m_partArray;
    
    void forEachPart(PartCallback& c, const CFrame& parentFrame, Part* part);

    /** Called from cleanGeometry */
    void computePartBounds();

    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);

    void load(const Specification& specification);

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
     */
    void cleanGeometry(const CleanGeometrySettings& settings);

    /** Appends one posed model per sub-part with geometry.

        Poses an object with no motion (see the other overloaded
        version for expressing motion)
    */
    void pose
    (Array<Surface::Ref>&     surfaceArray,
     const CoordinateFrame&   cframe = CoordinateFrame(),
     const Pose&              pose   = defaultPose());

    void pose
    (Array<Surface::Ref>&     surfaceArray,
     const CoordinateFrame&   cframe,
     const Pose&              pose,
     const CoordinateFrame&   prevCFrame,
     const Pose&              prevPose);
    

    /**
       Returns true if ray \a R intersects this model, when it has \a
       cframe and \a pose, at a distance less than \a maxDistance.  If
       so, sets maxDistance to the intersection distance and sets the
       pointers to the Part and Mesh, and the index in Mesh::cpuIndexArray of the 
       start of that triangle's indices.  \a u and \a v are the
       barycentric coordinates of vertices triStartIndex and triStartIndex + 1.
       The barycentric coordinate of vertex <code>triStartIndex + 2</code>
       is <code>1 - u - v</code>.

       This is primarily intended for mouse selection.  For ray tracing
       or physics, consider G3D::TriTree instead.
     */
    bool intersect
        (const Ray&     R, 
        const CFrame&   cframe, 
        const Pose&     pose, 
        float&          maxDistance, 
        Part*&          part, 
        Mesh*&          mesh, 
        int&            triStartIndex, 
        float&          u, 
        float&          v) const;
};

#endif
