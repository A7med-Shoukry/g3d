#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/VertexRange.h"
#include "G3D/CoordinateFrame.h"

namespace G3D {


void CPUVertexArray::Vertex::transformBy(const CoordinateFrame& cframe){
	position  = cframe.pointToWorldSpace(position);
	normal    = cframe.vectorToWorldSpace(normal);
	// The w component is just packed in.
	tangent   = Vector4(cframe.vectorToWorldSpace(tangent.xyz()), tangent.w);
}

CPUVertexArray::CPUVertexArray(const CPUVertexArray& otherArray) : hasTexCoord0(otherArray.hasTexCoord0), 
															hasTexCoord1(otherArray.hasTexCoord1), 
															hasTangent(otherArray.hasTangent) {

	vertex.copyPOD(otherArray.vertex);
	texCoord1.copyPOD(otherArray.texCoord1);
}

void CPUVertexArray::copyFrom(const CPUVertexArray& other){
	vertex.copyPOD(other.vertex);
	texCoord1.copyPOD(other.texCoord1);
	hasTexCoord0 = other.hasTexCoord0; 
	hasTexCoord1 = other.hasTexCoord1;
	hasTangent   = other.hasTangent;
}


void CPUVertexArray::transformAndAppend(const CPUVertexArray& otherArray, const CFrame& cframe){
	if(otherArray.hasTexCoord1){
		texCoord1.appendPOD(otherArray.texCoord1);
	}
	const int oldSize = vertex.size();
	vertex.appendPOD(otherArray.vertex);
	for(int i = oldSize; i < vertex.size(); ++i){
		vertex[i].transformBy(cframe);
	}

}


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
