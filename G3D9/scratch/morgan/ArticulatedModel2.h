#ifndef GLG3D_ArticulatedModel2_h
#define GLG3D_ArticulatedModel2_h

#include <G3D/G3DAll.h>


/**

Sample use cases:

I have an OBJ file with vertex normals, vertices, texcoords, and groups.
I want to load it and compute tangents, and then collapse

*/

class ArticulatedModel2 : public ReferenceCountedObject {
public:

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
        Material::Ref               material;

        PrimitiveType               primitive;
        Array<int>                  cpuIndexArray;
        VertexRange                 gpuIndexArray;

        bool                        twoSided;

        /** Relative to the Part containing it. */
        Sphere                      boundingSphere;

        Mesh() : primitive(PrimitiveType::TRIANGLES), twoSided(false) {}
    };

    /** 
     A set of meshes with a single reference frame, packed into a common vertex buffer.
    */
    class Part {
    private:
        Part*                       m_parent;
        Array<Part*>                m_child;

    public:

        /** Transformation from this object to the parent's frame in the rest pose */
        CFrame                      cframe;
        Sphere                      boundingSphere;

        Array<Point3>               cpuVertexArray;
        Array<Vector3>              cpuNormalArray;
        Array<Point2>               cpuTexCoord0Array;
        Array<Vector4>              cpuTangentArray;

        VertexRange                 gpuVertexArray;
        VertexRange                 gpuNormalArray;
        VertexRange                 gpuTexCoord0Array;
        VertexRange                 gpuTangentArray;

        /** NULL if this is a root of the model. */
        const Part* parent() const;
        const Array<Part*>& childArray() const;
    };

private:

    Array<Part*>                    m_rootArray;
    Array<Part*>                    m_partArray;
public:

    /** Root parts.  There may be more than one. */
    const Array<Part*>& rootArray() const;

    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);

    void load(const Specification& specification);
};

#endif
