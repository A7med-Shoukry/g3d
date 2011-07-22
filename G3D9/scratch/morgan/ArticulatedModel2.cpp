#include "ArticulatedModel2.h"


const ArticulatedModel2::Pose& ArticulatedModel2::defaultPose() {
    static const Pose p;
    return p;
}


void ArticulatedModel2::forEachPart(PartCallback& callback, const CFrame& parentFrame, Part* part) {
    // Net transformation from part to world space
    const CFrame& net = parentFrame * part->cframe;

    // Process all children
    for (int c = 0; c < part->m_child.size(); ++c) {
        forEachPart(callback, net, part->m_child[c]);
    }

    // Invoke the callback on this part
    callback(Ref(this), part, parentFrame);
}


void ArticulatedModel2::forEachPart(PartCallback& callback) {
    for (int p = 0; p < m_rootArray.size(); ++p) {
        forEachPart(callback, CFrame(), m_rootArray[p]);
    }
}


class AMTransform : public ArticulatedModel2::PartCallback {
    Matrix4 xform, normalXForm;

public:
    AMTransform(const Matrix4& xform) : xform(xform), normalXForm(xform.upper3x3().inverse().transpose()) {}

    virtual void operator()(ArticulatedModel2::Ref m, ArticulatedModel2::Part* part, const CFrame& parentFrame) override {

        Matrix4 vertexTransform;
        Matrix3 normalTransform;

        if (part->isRoot()) {
            alwaysAssertM(false, "Not implemented");
            // TODO
        } else {
            alwaysAssertM(false, "Not implemented");
            // Don't translate this part
        }

        for (int v = 0; v < part->cpuVertexArray.size(); ++v) {
            ArticulatedModel2::Vertex& vertex = part->cpuVertexArray[v];
            vertex.position = vertexTransform.homoMul(vertex.position, 1.0f);
            vertex.normal   = (normalTransform * vertex.normal).directionOrZero();
        }
    }
}; 


void ArticulatedModel2::load(const Specification& specification) {

    if (endsWith(toLower(specification.filename), ".obj")) {
        loadOBJ(specification);
    } else {
        // Error
        throw std::string("Unrecognized file extension on \"") + specification.filename + "\"";
    }

    //transform as demanded by the specification
    if (specification.xform != Matrix4::identity()) {
        AMTransform transform(specification.xform);
        forEachPart(transform);
    }

    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    cleanGeometry(specification.cleanGeometrySettings);
}


void ArticulatedModel2::pose
(Array<Surface::Ref>&     surfaceArray,
 const CoordinateFrame&   cframe,
 const Pose&              ppose) {

    pose(surfaceArray, cframe, ppose, cframe, ppose);
}


void ArticulatedModel2::pose
(Array<Surface::Ref>&     surfaceArray,
 const CoordinateFrame&   cframe,
 const Pose&              pose,
 const CoordinateFrame&   prevCFrame,
 const Pose&              prevPose) {

     for (int p = 0; p < m_rootArray.size(); ++p) {
         m_rootArray[p]->pose(surfaceArray, cframe, pose, prevCFrame, prevPose);
     }
}


void ArticulatedModel2::Part::pose
(Array<Surface::Ref>&     surfaceArray,
 const CoordinateFrame&   parentFrame,
 const Pose&              posex,
 const CoordinateFrame&   prevParentFrame,
 const Pose&              prevPose) {
    
    CFrame frame = parentFrame * cframe;
    CFrame prevFrame = prevParentFrame * cframe;

    CFrame* poseFramePtr = posex.cframe.getPointer(name);
    if (poseFramePtr) {
        frame = frame * posex.cframe[name];
    }

    poseFramePtr = prevPose.cframe.getPointer(name);
    if (poseFramePtr) {
        prevFrame = prevFrame * prevPose.cframe[name];
    }

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    // Pose the meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];
        alwaysAssertM(false, "TODO");
    }

    // Pose the children
    for (int c = 0; c < m_child.size(); ++c) {
        m_child[c]->pose(surfaceArray, frame, posex, prevFrame, prevPose);
    }
}


bool ArticulatedModel2::intersect
    (const Ray&     R, 
    const CFrame&   cframe, 
    const Pose&     pose, 
    float&          maxDistance, 
    Part*&          part, 
    Mesh*&          mesh, 
    int&            triStartIndex, 
    float&          u, 
    float&          v) const {

    alwaysAssertM(false, "TODO");
}
