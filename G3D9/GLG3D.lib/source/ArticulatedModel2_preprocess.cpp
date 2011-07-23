#include "GLG3D/ArticulatedModel2.h"

namespace G3D {
    
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
            CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[v];
            vertex.position = vertexTransform.homoMul(vertex.position, 1.0f);
            vertex.normal   = (normalTransform * vertex.normal).directionOrZero();
        }
    }
}; 


class AMScaleTransform : public ArticulatedModel2::PartCallback {
    float scaleFactor;

public:
    AMScaleTransform(float s) : scaleFactor(s) {}

    virtual void operator()(ArticulatedModel2::Ref m, ArticulatedModel2::Part* part, const CFrame& parentFrame) override {
        part->cframe.translation *= scaleFactor;

        const int N = part->cpuVertexArray.size();
        CPUVertexArray::Vertex* ptr = part->cpuVertexArray.vertex.getCArray();
        for (int v = 0; v < N; ++v) {
            ptr[v].position *= scaleFactor;
        }
    }
}; 


void ArticulatedModel2::preprocess(const Array<Instruction>& program) {
    for (int i = 0; i < program.size(); ++i) {
        const Instruction& instruction = program[i];

        switch (instruction.type) {
        case Instruction::SCALE:
            {
                // Scale every pivot translation and every vertex position by the scale factor
                const float scaleFactor = instruction.arg.number();
                AMScaleTransform transform(scaleFactor);
                forEachPart(transform);
            }
            break;

        default:
            alwaysAssertM(false, "Instruction not implemented");
        }
    }
}


} // namespace
