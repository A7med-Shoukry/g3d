#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/VertexRange.h"

namespace G3D {

void CPUVertexArray::copyToGPU
(VertexRange&               vertexVR, 
 VertexRange&               normalVR, 
 VertexRange&               packedTangentVR, 
 VertexRange&               texCoord0VR,
 VertexRange&               texCoord1VR,
 VertexBuffer::UsageHint    hint) const {

#   define OFFSET(field) ((size_t)(&dummy.field) - (size_t)&dummy)

    const int numVertices = size();
    if (numVertices > 0) {
        int byteSize = sizeof(Vertex) * numVertices;
        
        if (hasTexCoord1) { 
            byteSize += sizeof(Vector2unorm16) * numVertices;
        }

        const int padding = 16;

        VertexBuffer::Ref buffer = VertexBuffer::create(byteSize + padding, VertexBuffer::WRITE_ONCE);
        
        VertexRange all(byteSize, buffer);

        Vertex dummy;
        vertexVR        = VertexRange(dummy.position,  numVertices, all, OFFSET(position),  (int)sizeof(Vertex));
        normalVR        = VertexRange(dummy.normal,    numVertices, all, OFFSET(normal),    (int)sizeof(Vertex));
        packedTangentVR = VertexRange(dummy.tangent,   numVertices, all, OFFSET(tangent),   (int)sizeof(Vertex));
        texCoord0VR     = VertexRange(dummy.texCoord0, numVertices, all, OFFSET(texCoord0), (int)sizeof(Vertex));

        if (hasTexCoord1) {
            texCoord1VR = VertexRange(texCoord1, buffer);
        } else {
            texCoord1VR = VertexRange();
        }

        // Copy all interleaved data at once
        Vertex* dst = (Vertex*)all.mapBuffer(GL_WRITE_ONLY);

        System::memcpy(dst, vertex.getCArray(), byteSize);

        all.unmapBuffer();
        dst = NULL;
    }

#undef OFFSET
}

} // G3D
