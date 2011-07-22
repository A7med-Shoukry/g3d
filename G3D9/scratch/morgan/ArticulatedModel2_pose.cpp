#include "ArticulatedModel2.h"


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

    // TODO: Force loading/copying assets to the GPU if not already done

    // Pose the meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];

        alwaysAssertM(false, "TODO: Create a SuperSurface here");
    }

    // Pose the children
    for (int c = 0; c < m_child.size(); ++c) {
        m_child[c]->pose(surfaceArray, frame, posex, prevFrame, prevPose);
    }
}
