#ifndef GLG3D_ArticulatedModel2_h
#define GLG3D_ArticulatedModel2_h

#include <G3D/G3DAll.h>


/**


*/
class ArticulatedModel2 : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<ArticulatedModel2> Ref;

    class Specification {
    public:
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
            Vertex      vertex[3];
            int         index[3];
            Face() {
                index[0] = index[1] = index[2] = -1;
            }
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
        void cleanGeometry();
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

    /** Root parts.  There may be more than one. */
    const Array<Part*>& rootArray() const;

    Mesh* addMesh(const std::string& name, Part* part) {
        part->m_meshArray.append(new Mesh(name));
        return part->m_meshArray.last();
    }

    Part* addPart(const std::string& name, Part* parent = NULL) {
        m_partArray.append(new Part(name, parent));
        if (parent == NULL) {
            m_rootArray.append(m_partArray.last());
        }
        return m_partArray.last();
    }

    /** Walks the hierarchy and invokes PartCallback on each element. */
    void forEachPart(PartCallback& c);
    
    /** 
     Invokes Part::cleanGeometry on all parts.
    */
    void cleanGeometry();

private:

    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);

    void load(const Specification& specification);
};

#endif
