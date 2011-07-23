/**
 \file GLG3D/source/ArticulatedModel2_pose.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-16
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel2.h"

namespace G3D {
    
const ArticulatedModel2::Pose& ArticulatedModel2::defaultPose() {
    static const Pose p;
    return p;
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
         m_rootArray[p]->pose(Ref(this), surfaceArray, cframe, pose, prevCFrame, prevPose);
     }
}


void ArticulatedModel2::Part::pose
(const ArticulatedModel2::Ref& model,
 Array<Surface::Ref>&     surfaceArray,
 const CoordinateFrame&   parentFrame,
 const Pose&              posex,
 const CoordinateFrame&   prevParentFrame,
 const Pose&              prevPose) {
    
    CFrame frame = parentFrame * cframe * posex[name];
    CFrame prevFrame = prevParentFrame * cframe * prevPose[name];

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    if ((cpuVertexArray.size() > 0) && ! gpuPositionArray.valid()) {
        copyToGPU();
    }

    // Pose the meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];

        const SuperSurface::CPUGeom cpuGeom(&mesh->cpuIndexArray, &cpuVertexArray);
        SuperSurface::Ref surface = SuperSurface::create(name + "/" + mesh->name, frame, prevFrame, mesh->gpuGeom, cpuGeom, model);

        surfaceArray.append(surface);
    }

    // Pose the children
    for (int c = 0; c < m_child.size(); ++c) {
        m_child[c]->pose(model, surfaceArray, frame, posex, prevFrame, prevPose);
    }
}


void ArticulatedModel2::Part::copyToGPU() {
    cpuVertexArray.copyToGPU(gpuPositionArray, gpuNormalArray, gpuTangentArray, gpuTexCoord0Array);

    // If fewer than 2^16 vertices, switch to uint16 indices
    const size_t indexBytes = (cpuVertexArray.size() < (1<<16)) ? sizeof(uint16) : sizeof(int);
    VertexBuffer::Ref all = VertexBuffer::create(m_triangleCount * 3 * indexBytes + 16, VertexBuffer::WRITE_ONCE, VertexBuffer::INDEX);

    for (int m = 0; m < m_meshArray.size(); ++m) {
        Mesh* mesh = m_meshArray[m];
        if (indexBytes == 2) {
            // Explicitly map and convert to 16-bit indices
            uint16 dummy = 0;
            VertexRange temp(mesh->cpuIndexArray.size() * indexBytes, all);
            int N = mesh->cpuIndexArray.size();
            mesh->gpuIndexArray = VertexRange(dummy, N, temp, 0, sizeof(uint16));
            const int32* src = mesh->cpuIndexArray.getCArray();
            uint16* dst = (uint16*)mesh->gpuIndexArray.mapBuffer(GL_WRITE_ONLY);
            for (int i = 0; i < N; ++i) {
                dst[i] = (uint16)src[i];
            }
            mesh->gpuIndexArray.unmapBuffer();
        } else {
            // Directly copy the 32-bit indices
            mesh->gpuIndexArray = VertexRange(mesh->cpuIndexArray, all);
        }

        // Create the corresponding GPU geometry
        mesh->gpuGeom = SuperSurface::GPUGeom::create(mesh->primitive);
        mesh->gpuGeom->boxBounds = mesh->boxBounds;
        mesh->gpuGeom->sphereBounds = mesh->sphereBounds;
        mesh->gpuGeom->index     = mesh->gpuIndexArray;
        mesh->gpuGeom->material  = mesh->material;
        mesh->gpuGeom->vertex    = gpuPositionArray;
        mesh->gpuGeom->normal    = gpuNormalArray;
        mesh->gpuGeom->packedTangent = gpuTangentArray;
        mesh->gpuGeom->texCoord0 = gpuTexCoord0Array;
    }
}

} // namespace G3D

