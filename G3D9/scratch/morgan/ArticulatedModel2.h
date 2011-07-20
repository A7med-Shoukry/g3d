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
        Matrix4                     xform;
        bool                        stripMaterials;
    };

    class Mesh {
    public:

    };

    /** 
     A set of meshes with a single reference frame, packed into a common vertex buffer.
    */
    class Part {
    public:

        const Part* parent() const;
        const Array<Part*>& childArray() const;

    };

    /** Root parts */
    const Array<Part*>& rootArray() const;

    /** Create a new part.
      \param parent        May be NULL, if a root part.
      \param bodyToParent  Transformation from the part's reference frame to the parent's reference frame.
      \return The new part, which has also been added to the model.
      */
    Part* addPart
       (const std::string&              name, 
        Part*                           parent, 
        const CFrame&                   bodyToParent, 
        Array<Point3>&                  vertex,
        Array<Vector3>&                 normal,
        Array<Point2>&                  texCoord,
        Array<Vector4>&                 tangent,
        Array<int>&                     triList,
        Array<int>&                     oldToNew);

    void addMesh
        (const std::string&             name,
         Part*                          part,
         PrimitiveType                  type,
         const Array<int>&              index,
         const Material::Specification& materialSpec);

    void loadOBJ(const Specification& specification);
};

#endif
