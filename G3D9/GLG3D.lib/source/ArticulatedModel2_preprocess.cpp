#include "GLG3D/ArticulatedModel2.h"

namespace G3D {
    

class AMScaleTransform : public ArticulatedModel2::PartCallback {
    float scaleFactor;

public:
    AMScaleTransform(float s) : scaleFactor(s) {}

    virtual void operator()(ArticulatedModel2::Part* part, const CFrame& parentFrame, ArticulatedModel2::Ref m) override {
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
