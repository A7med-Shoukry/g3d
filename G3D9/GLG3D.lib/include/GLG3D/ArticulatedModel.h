/**
 \file GLG3D/ArticulatedModel.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2011-08-26
 
 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_ArticulatedModel_h
#define GLG3D_ArticulatedModel_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Matrix4.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Sphere.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/constants.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/Material.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/Surface.h"
#include "GLG3D/SuperSurface.h"

namespace G3D {

/**
 \brief A 3D object composed of multiple rigid triangle meshes connected by joints.

 Supports the following file formats:

 - <a href="http://www.martinreddy.net/gfx/3d/OBJ.spec">OBJ</a> + <a href="http://www.fileformat.info/format/material/">MTL</a>
 - <a href="http://paulbourke.net/dataformats/ply/">PLY</a>
 - <a href="http://g3d.svn.sourceforge.net/viewvc/g3d/data/ifs/fileformat.txt?revision=27&view=markup">IFS</a> 
 - <a href="http://www.geomview.org/docs/html/OFF.html">OFF</a>
 - <a href="http://www.the-labs.com/Blender/3DS-details.html">3DS</a>
 - <a href="http://www.riken.jp/brict/Yoshizawa/Research/PLYformat/PLYformat.html">PLY2</a>
 - Quake 3 <a href="http://www.mralligator.com/q3/">BSP</a>
 - Any image format supported by G3D::GImage will be treated as a heighfield; apply a scale factor and material to transform it as desired

 Does not copy geometry to the GPU until it has to render.  This means
 that CPU rendering code need not consume GPU vertex buffer resources
 (or transfer time).  The current implementation eagerly loads
 Material%s onto the GPU.  If a future version also allows deferring
 that operation, then this class will be able to load models entirely
 on a non-rendering thread and will not require a GPU context to be
 active.
 */
class ArticulatedModel : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<ArticulatedModel> Ref;

    /** Parameters for cleanGeometry() */
    class CleanGeometrySettings {
    public:
        
        /** 
            Set to true to check for redundant vertices even if no
            normals or tangents need to be computed. This may increase
            rendering performance and decrease cleanGeometry()
            performance.  Default: true.
        */
        bool                        forceVertexMerging;


        /** 
            Set to false to prevent the (slow) operation of merging
            colocated vertices that have identical properties.
            Merging vertices speeds up rendering but slows down
            loading.  Setting to false overrides
            forceVertexMerging. */
        bool                        allowVertexMerging;

        /**
           Maximum angle in radians that a normal can be bent through
           to merge two vertices.  Default: 8 degrees().
          */
        float                       maxNormalWeldAngle;

        /** 
            Maximum angle in radians between the normals of adjacent
            faces that will still create the appearance of a smooth
            surface between them.  Alternatively, the minimum angle
            between those normals required to create a sharp crease.

            Set to 0 to force faceting of a model.  Set to 2 * pif()
            to make completely smooth.

            Default: 65 degrees().
        */
        float                       maxSmoothAngle;

        CleanGeometrySettings() : 
            forceVertexMerging(true),
            allowVertexMerging(true),
            maxNormalWeldAngle(8 * units::degrees()),
            maxSmoothAngle(65 * units::degrees()) {
        }

        CleanGeometrySettings(const Any& a);

        Any toAny() const;
    };

    /** \brief Unique identifier within a particular model a Part or a Mesh. 
     You can use the G3D viewer tool on a model to interactively discover its
     part and mesh ID%s.*/
    class ID {
    private:
        int     m_value;

    public:

        ID() : m_value(0) {}

        explicit ID(int i) : m_value(i) {}

        ID& operator=(const ID& i) {
            m_value = i;
            return *this;
        }

        bool operator==(const ID& other) const {
            return m_value == other.m_value;
        }

        bool operator!=(const ID& other) const {
            return m_value != other.m_value;
        }

        operator int() const {
            return m_value;
        }

        bool initialized() const {
            return m_value != 0;
        }

        size_t hashCode() const {
            return HashTrait<int>::hashCode(m_value);
        }

        static size_t hashCode(const ID& id) {
            return id.hashCode();
        }
    };


    /** \brief Preprocessing instruction. 
        \sa G3D::Specification::Specification
    */
    class Instruction {
    private:
        friend class ArticulatedModel;

        enum Type {SCALE, MOVE_CENTER_TO_ORIGIN, MOVE_BASE_TO_ORIGIN, SET_CFRAME, TRANSFORM_CFRAME, 
                   TRANSFORM_GEOMETRY, DELETE_MESH, DELETE_PART, SET_MATERIAL, SET_TWO_SIDED, 
                   MERGE_ALL, RENAME_PART, RENAME_MESH, ADD};

        /**
          An identifier is one of:

          - all(): all parts in a model, or all meshes in a part, depending on context
          - root(): all root parts
          - a string that is the name of a mesh or part at this point in preprocessing
          - a positive integer that is the ID of a mesh or part before preprocessing occured.  Each part and mesh has a unique ID.  Mesh ID's are global to the model, not scoped within a Part.
          - 0, indicating that this is a Part ID to ignore because the mesh has an absolute ID
         */
        class Identifier {
        public:
            std::string             name;
            ID                      id;

            Identifier() {}
            Identifier(ID id) : id(id) {}
            Identifier(const std::string& name) : name(name) {}
            Identifier(const Any& a);

            bool isAll() const {
                return int(id) == -3;
            }

            bool isRoot() const {
                return int(id) == -2;
            }

            bool isNone() const {
                return int(id) == -4;
            }

            /** No part */
            static Identifier none() {
                return Identifier(ID(-4));
            }

            /** All root Part%s */
            static Identifier root() {
                return Identifier(ID(-2));
            }

            /** All Part%s, or all Mesh%es in a part, depending on context */
            static Identifier all() {
                return Identifier(ID(-3));
            }

            Any toAny() const;
        };

        Type                        type;
        Identifier                  part;
        Identifier                  mesh;
        Any                         arg;

        Any                         source;

    public:

        Instruction() : type(SCALE) {}

        Instruction(const Any& a);

        Any toAny() const;
    };


    /** \brief Parameters for constructing a new ArticulatedModel from
        a file on disk. */
    class Specification {
    public:
        /** Materials will be loaded relative to this file.*/
        std::string                 filename;

        /** Ignore materials specified in the file, replacing them
            with Material::create().  Setting to true increases
            loading performance and may allow more aggressive
            optimization if mergeMeshesByMaterial is also true.
        */
        bool                        stripMaterials;

        /** Merge all Mesh%es within a Part that share a material into
            a single Mesh.  This will increase the rendering
            performance of objects that were divided into many parts
            by an artist for modeling purposes.  It may decrease the
            rendering performance of very large objects that were
            divided into pieces that are unlikely to all be visible in
            the frustum simultaneously.

            Default: false
        */
        bool                        mergeMeshesByMaterial;

        /** Multiply all vertex positions and part translations by
            this factor after loading and before preprocessing.
            Default = 1.0. */
        float                       scale;

        CleanGeometrySettings       cleanGeometrySettings;

        /** A program to execute to preprocess the mesh before
            cleaning geometry. */
        Array<Instruction>          preprocess;


        class BSPOptions {
        public:
            /** If true, lightmap coordinates are used for texCoord1.
                If false (default), they are discarded.
              */
            bool                    preserveLightMapCoordinates;

            BSPOptions() : preserveLightMapCoordinates(false) {}
        } bsp;


        class OBJOptions {
        public:

            enum TexCoord3DMode {
                /** Ignore the w component of a 3D texture coordinate (default) */
                IGNORE,

                /** Compute 
                    \code
                      texCoord1.x = floor(w / (2.0f * 2048.0f)) / 2048.0f
                      texCoord1.y = (w - 2.0f * 2048.0f * floor(w / (2.0f * 2048.0f))) / 2048.0f 
                    \endcode

                    This format allows third-party programs to
                    preserve the texture coordinate during processing.
                    The constant 2048 is chosen based on the internal
                    precision of texture coordinates in 3DS Max.
                  */
                UNIT_2048,

                /** 
                  Parse texture coordinates as (x0, y0, x1, y1).  This
                  is nonstandard and will not allow most programs to
                  preserve texture coordinates.  However, most
                  programs will also ignore the 3rd and 4th coordinate
                  on load and thus the format is backwards compatible.
                */
                FOUR_EXTENSION
            };

            TexCoord3DMode  texCoord3DMode;

            OBJOptions() : texCoord3DMode(IGNORE) {}

        } obj;

        Specification() : stripMaterials(false), mergeMeshesByMaterial(false), scale(1.0f) {}

        /**
        Example:
        \code
        ArticulatedModel::Specification {
            filename = "house.obj";
            mergeMeshesByMaterial = true;
            stripMaterials = false;
            scale = 0.5;
            preprocess = (
                // Set the reference frame of a part, relative to its parent
                // All parts and meshes may be referred to by name string or ID integer
                // in any instruction.   Use partID = 0 when using a mesh ID.
                setCFrame("fence", CFrame::fromXYZYPRDegrees(0, 13, 0));

                // Scale the entire object, including pivots, by *another* factor of 0.1
                scale(0.1);

                // Add this model as a new root part
                add(ArticulatedModel::Specification {
                   filename = "dog.obj";
                   preprocess = ( renamePart(root(), "dog"); );
                });

                // Add this model as a new part, as a child of root()
                add(root(), ArticulatedModel::Specification {
                   filename = "cat.obj";
                });

                transformCFrame(root(), CFrame::fromXYZYPRDegrees(0,0,0,90));

                // Transform the root part translations and geometry
                // so that the center of the bounding box in the
                // default pose is at the origin.
                moveCenterToOrigin();

                moveBaseToOrigin();

                // Apply a transformation to the vertices and child
                // translations of a part, within its reference frame
                transformGeometry("root", Matrix4::scale(0, 1, 2));

                // Remove a mesh.  This does not change the ID's of other meshes
                deleteMesh("fence", "gate");

                // Remove a part.  This does not change the ID's of other parts
                deletePart("porch");

                // Replace the material of a Mesh
                setMaterial("chair", "woodLegs", Material::Specification { lambertian = Color3(0.5); });

                // Change the two-sided flag
                setTwoSided("window", "glass", true);

                // Move all Meshes into a single root Part
                mergeAll();

                renamePart("x17", "television");

                renameMesh("foo", "bar", "baz");
            );
        }
        \endcode
         */
        Specification(const Any& a);

        Any toAny() const;
    };


    /** 
        \brief A set of primitives that share a material, within a Part.
     */
    class Mesh {
    public:
        friend class ArticulatedModel;

        std::string                 name;

        const ID                    id;

        Material::Ref               material;

        PrimitiveType               primitive;
        Array<int>                  cpuIndexArray;
        VertexRange                 gpuIndexArray;

        bool                        twoSided;

        /** Relative to the Part containing it. */
        Sphere                      sphereBounds;

        /** Relative to the Part containing it. */
        AABox                       boxBounds;

        /** Written by Part::copyToGPU */
        SuperSurface::GPUGeom::Ref  gpuGeom;

    private:
        
        Mesh(const std::string& name, ID id) : 
            name(name), id(id), primitive(PrimitiveType::TRIANGLES), twoSided(false) {}

    };


    /** Specifies the transformation that occurs at each node in the heirarchy. 
     */
    class Pose {
    private:
        static const CFrame identity;
    public:
        /** Mapping from names to coordinate frames (relative to parent).
            If a name is not present, its coordinate frame is assumed to
            be the identity. */
        Table<std::string, CoordinateFrame>     cframe;

        /** True if this model casts shadows in this pose. Default is true. */
        bool                                    castsShadows;

        Pose() : castsShadows(true) {}

        /** Returns the identity coordinate frame if there isn't one bound for partName */
        inline const CFrame& operator[](const std::string& partName) const {
            CFrame* ptr = cframe.getPointer(partName);
            if (ptr != NULL) {
                return *ptr;
            } else {
                return identity;
            }
        }
    };


    class PoseSpline {
    public:
        typedef Table<std::string, PhysicsFrameSpline> SplineTable;
        SplineTable     partSpline;
        bool            castsShadows;

        PoseSpline();

        /**
         The Any must be a table mapping part names to PhysicsFrameSpline%s.
         Note that a single PhysicsFrame (or any equivalent of it) can serve as
         to create a PhysicsFrameSpline.  

         Format example:
         \code
         PoseSpline {
                "part1" = PhysicsFrameSpline {
                   control = ( Vector3(0,0,0),
                               CFrame::fromXYZYPRDegrees(0,1,0,35)),
                   cyclic = true
                };

                "part2" = Vector3(0,1,0);

                castsShadows = true;
            }
         \endcode
        */
        PoseSpline(const Any& any);
     
        /** Get the pose at time t, overriding values in \a pose that
            are specified by the spline. */
        void get(float t, ArticulatedModel::Pose& pose);
    };


    /** 
     \brief A set of meshes with a single reference frame, packed into
     a common vertex buffer.
    */
    class Part {
    private:
        friend class ArticulatedModel;
       
        /** Used by cleanGeometry */
        class Face {
        public:

            /** Index of a Face in a temporary array*/
            typedef int                         Index;
            typedef SmallArray<Index, 7>        IndexArray;
            typedef Table<Point3, IndexArray>   AdjacentFaceTable;

            CPUVertexArray::Vertex              vertex[3];

            /** Mesh from which this face was originally created. Needed for reconstructing
              the index arrays after vertices are merged.*/
            Mesh*                               mesh;
            
            /** Non-unit face normal, used for weighted vertex normal computation */
            Vector3                             normal;
            Vector3                             unitNormal;

            Face() : mesh(NULL) {}
        };

    public:

        std::string                 name;
        const ID                    id;

    private:

        bool                        m_hasTexCoord0;
        bool                        m_hasTexCoord1;
        Part*                       m_parent;
        Array<Part*>                m_child;
        Array<Mesh*>                m_meshArray;
        int                         m_triangleCount;

    public:

        /** Transformation from this object to the parent's frame in
            the rest pose. Also known as the "pivot". */
        CFrame                      cframe;

        /** Bounding sphere of just this Part's geometry, in object
            space. Does not include child parts.*/
        Sphere                      sphereBounds;

        /** Bounding box of just this Part's geometry, in object
            space. Does not include child parts.*/
        AABox                       boxBounds;

        /** \brief The geometry for this part, accessible on the CPU side.
            
            If you change this and want the geometry to be "cleaned" for you,
            invoke cleanGeometry().

            After changing this data, invoke clearVertexRanges() to
            invalidate the old copy on the GPU.
         */
        CPUVertexArray              cpuVertexArray;

        VertexRange                 gpuPositionArray;
        VertexRange                 gpuNormalArray;
        VertexRange                 gpuTexCoord0Array;
        VertexRange                 gpuTangentArray;
        VertexRange                 gpuTexCoord1Array;

    private:
        
        /** Called from cleanGeometry to determine what needs to be computed. */
        void determineCleaningNeeds(bool& computeSomeNormals, bool& computeSomeTangents);

        /** Called from cleanGeometry */
        void buildFaceArray(Array<Face>& faceArray, Face::AdjacentFaceTable& adjacentFaceTable);

        /** Called from cleanGeometry.  Computes all vertex normals
            that are currently NaN. */
        void computeMissingVertexNormals
         (Array<Face>&                      faceArray, 
          const Face::AdjacentFaceTable&    adjacentFaceTable, 
          const float                       maximumSmoothAngle);

        /** Called from cleanGeometry.  Collapses shared vertices back
        into cpuVertexArray.  

        \param maxNormalWeldAngle Maximum amount a normal can be bent
        to merge two vertices. */
        void mergeVertices(const Array<Face>& faceArray, float maxNormalWeldAngle);

        /** Called from cleanGeometry(). Computes all tangents that
            are currently NaN.*/
        void computeMissingTangents();

        /** Uploads all data for this part and its meshes to the GPU.
            Does not affect children. */
        void copyToGPU();

        /** Transforms all vertex positions and child part translations.
            Erases all data except texcoords and vertices. */
        void transformGeometry(const Matrix4& xform);

        Part(const std::string& name, Part* parent, ID id) : name(name), id(id), m_parent(parent) {}

    public:

        /** Discards all GPU vertex range data, which will then be
            re-uploaded automatically on the next pose() call. 

            Invoke this after you modify cpuVertexArray.
        */
        void clearVertexRanges();

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
        - If there are texture coordinates, computes a tangent for every element whose tangent.x is nanf() (or if the tangent array is empty).
        - Merges all vertices with identical indices.
        - Updates all Mesh indices accordingly.
        - Recomputes the bounding sphere.
        
        Does not upload to the GPU.  You must invoke clearVertexRanges afterward.

        */
        void cleanGeometry(const CleanGeometrySettings& settings);

        /** Pose this part and all of its children */
        void pose
            (const ArticulatedModel::Ref& model,
             Array<Surface::Ref>&     surfaceArray,
             const CoordinateFrame&   parentFrame,
             const Pose&              pose,
             const CoordinateFrame&   prevParentFrame,
             const Pose&              prevPose);

        /** debugPrintf all of the geometry for this part. */
        void debugPrint() const;

    };


    /** Base class for defining operations to perform on each part, in hierarchy order.
    
    Example:
    \code

    class ExtractVertexCallback : public ArticulatedModel::PartCallback {
    public:
        Array<Point3>& vertexArray;

        ExtractVertexCallback(Array<Point3>& vertexArray) : vertexArray(vertexArray) {}

        void operator()(ArticulatedModel::Part *part, const CFrame &worldToPartFrame, ArticulatedModel::Ref model, const int treeDepth) {
            for (int i = 0; i < part->cpuVertexArray.size(); ++i) {
                vertexArray.append(worldToPartFrame.pointToObjectSpace(part->cpuVertexArray.vertex[i].position));
            }
        }
    } callback(vertexArray);

    model->forEachPart(callback);
    \endcode
    */
    class PartCallback {
    public:
        /** \brief Override to implement processing of \a part. 
            
            \param worldToPartFrame The net transformation in this
            pose from world space to \a part's object space

            \param treeDepth depth in the hierarchy.  0 = a root
        */
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
                                ArticulatedModel::Ref model, const int treeDepth) {}
    };


    /** Computes the world-space bounds of this model. */
    class BoundsCallback : public ArticulatedModel::PartCallback {
    public:
        
        AABox bounds;
        
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
                                ArticulatedModel::Ref m, const int treeDepth) override;
    };


    /** Rescales each part (and the position of its cframe) by a constant factor. */
    class ScaleTransformCallback : public ArticulatedModel::PartCallback {
        float scaleFactor;
        
    public:
        ScaleTransformCallback(float s) : scaleFactor(s) {}
        
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& partFrame,
                                ArticulatedModel::Ref m, const int treeDepth) override;
    };


    /** The rest pose.*/
    static const Pose& defaultPose();

    std::string                     name;

protected:

    Array<Part*>                    m_rootArray;
    Array<Part*>                    m_partArray;

    /** Used for allocating IDs */            
    int                             m_nextID;

    /** Allocate a new ID */
    ID createID() {
        const ID i = ID(m_nextID);
        ++m_nextID;
        return i;
    }

    Table<ID, Part*, ID>            m_partTable;
    Table<ID, Mesh*, ID>            m_meshTable;
    
    /** \brief Execute the program.  Called from load() */
    void preprocess(const Array<Instruction>& program);

    /** \brief Executes \a c for each part in the hierarchy.
     */
    void forEachPart(PartCallback& c, Part* part, const CFrame& parentFrame, const Pose& pose, const int treeDepth);

    /** Called from cleanGeometry */
    void computePartBounds();

    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);

    void loadIFS(const Specification& specification);

    void loadPLY2(const Specification& specification);

    void loadOFF(const Specification& specification);

    void loadPLY(const Specification& specification);

    void load3DS(const Specification& specification);

    void loadBSP(const Specification& specification);

    void loadHeightfield(const Specification& specification);

    void load(const Specification& specification);

    ArticulatedModel() : m_nextID(1) {}

    Mesh* mesh(const Instruction::Identifier& part, const Instruction::Identifier& mesh);

    Part* part(const Instruction::Identifier& partIdent);

    /** Called from preprocess.  

        \param centerY If false, move the base to the origin instead
        of the center in the vertical direction. */
    void moveToOrigin(bool centerY);

public:

    /** Leaves empty filenames alone and resolves others */
    static std::string resolveRelativeFilename(const std::string& filename, const std::string& basePath);

    /** \sa createEmpty, fromFile */
    static Ref create(const Specification& s);

    static Ref fromFile(const std::string& filename) {
        Specification s;
        s.filename = filename;
        return create(s);
    }

    /** If this model's memory footprint is large, trim all of the internal CPU arrays to size. */
    void maybeCompactArrays();

    /** 
        \brief Creates an empty ArticulatedModel that you can then programmatically construct Part%s and Mesh%es within.

        Consider calling cleanGeometry() and maybeCompactArrays()
        after setting geometry during a preprocessing step.  If
        modifying geometry <i>after</i> the first call to pose(), invoke
        Part::clearVertexRanges() to wipe the out of date GPU data.


        Example of a procedurally generated model (run on load; too slow to execute in an animation loop):

        \code
        ArticulatedModel::Ref    model = ArticulatedModel::createEmpty("spiral");
        ArticulatedModel::Part*  part  = model->addPart("root");
        ArticulatedModel::Mesh*  mesh  = model->addMesh("root", part);

        // Create the vertices
        for (int i = 0; i < 100; ++i) {
            CPUVertexArray::Vertex& v = part->cpuVertexArray.vertex.next();
            v.position = Point3( ... );
        }
        
        // Create the indices
        for (int i = 0; i < 50; ++i) {
            mesh->cpuIndexArray.append( ... );
        }

        // Tell Articulated model to generate normals automatically
        part->cleanGeometry();
        \endcode


        Example of updating geometry on the CPU for vertex animation
        (consider using a custom Shader to perform this work on the
        GPU if possible):

        \code
        Array<CPUVertexArray::Vertex>& vertex = model->part(partID);
        for (int i = 0; i < 50; ++i) {
            vertex[i].position = ... ;
            vertex[i].normal   = ... ;
        }
        part->clearVertexArrays();
        \endcode

        Note that you can obtain the partID when a part is originally
        created, by explicitly iterating through the hierarchy from 
        rootArray(), or by iterating with forEachPart().


        Consider creating a SuperSurface, your own Surface subclass,
        or using VertexBuffer directly if you need extensive dynamic
        geometry that doesn't fit the design of ArticulatedModel
        well.  G3D does not require the use of ArticulatedModel at all--
        it is just a helper class to jumpstart your projects.
        
        \sa create, fromFile, addMesh, addPart
       */
    static Ref createEmpty(const std::string& name);

    /** Root parts.  There may be more than one. */
    const Array<Part*>& rootArray() const {
        return m_rootArray;
    }
    
    /** Get a Mesh by name.  Returns NULL if there is no such mesh. */
    Mesh* mesh(const std::string& partName, const std::string& meshName);

    /** Get a Part by name.  Returns NULL if there is no such part. */
    Part* part(const std::string& partName);

    Mesh* mesh(const ID& id);

    Part* part(const ID& id);

    /** \sa addPart, createEmpty */
    Mesh* addMesh(const std::string& name, Part* part);

    /** \sa addMesh, createEmpty */
    Part* addPart(const std::string& name, Part* parent = NULL);

    /** Walks the hierarchy and invokes PartCallback \a c on each Part,
        where each model is in \a pose and the entire model is relative to
        \a cframe.

        Remember to call cleanGeometry() if you change the geometry to force 
        it to re-upload to the GPU.

        Remember to set any normals and tangents you want recomputed to NaN.
    */
    void forEachPart(PartCallback& c, const CFrame& cframe = CFrame(), const Pose& pose = defaultPose());
    
    /** 
      Invokes Part::cleanGeometry on all parts.       
     */
    void cleanGeometry(const CleanGeometrySettings& settings = CleanGeometrySettings());

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
      \brief Per-triangle ray-model intersection.

       Returns true if ray \a R intersects this model, when it has \a
       cframe and \a pose, at a distance less than \a maxDistance.  If
       so, sets maxDistance to the intersection distance and sets the
       pointers to the Part and Mesh, and the index in Mesh::cpuIndexArray of the 
       start of that triangle's indices.  \a u and \a v are the
       barycentric coordinates of vertices triStartIndex and triStartIndex + 1,
       The barycentric coordinate of vertex <code>triStartIndex + 2</code>
       is <code>1 - u - v</code>.

       This is primarily intended for mouse selection.  For ray tracing
       or physics, consider G3D::TriTree instead.

       Does not overwrite the arguments unless there is a hit closer than maxDistance.
     */
     // Not const because it returns non-const pointers to members
    bool intersect
       (const Ray&     R, 
        const CFrame&   cframe, 
        const Pose&     pose, 
        float&          maxDistance, 
        Part*&          part, 
        Mesh*&          mesh, 
        int&            triStartIndex, 
        float&          u, 
        float&          v);

    void countTrianglesAndVertices(int& tri, int& vert) const;
    
};

/** \deprecated Use ArticulatedModel */
typedef ArticulatedModel ArticulatedModel2;

}  // namespace G3D


#endif // GLG3D_ArticulatedModel_h
