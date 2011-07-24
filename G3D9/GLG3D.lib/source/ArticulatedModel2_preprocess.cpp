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

        Part* partPtr = NULL;
        Mesh* meshPtr = NULL;

        switch (instruction.type) {
        case Instruction::SCALE:
            {
                // Scale every pivot translation and every vertex position by the scale factor
                const float scaleFactor = instruction.arg.number();
                AMScaleTransform transform(scaleFactor);
                forEachPart(transform);
            }
            break;

        case Instruction::SET_MATERIAL:
            {
                Material::Ref material = Material::create(instruction.arg);
                if (instruction.part.isRoot()) {
                    instruction.arg.verify(instruction.mesh.isAll(), "part = root() requires mesh = all()");
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        partPtr = m_rootArray[p];
                        for (int m = 0; m < partPtr->m_meshArray.size(); ++m) {
                            partPtr->m_meshArray[m]->material = material;
                        }
                    }
                } else if (instruction.part.isAll()) {
                    instruction.arg.verify(instruction.mesh.isAll(), "part = all() requires mesh = all()");
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        partPtr = m_partArray[p];
                        for (int m = 0; m < partPtr->m_meshArray.size(); ++m) {
                            partPtr->m_meshArray[m]->material = material;
                        }
                    }
                } else {
                    meshPtr = mesh(instruction.part, instruction.mesh);
                    instruction.arg.verify(meshPtr != NULL, "Mesh not found in Part.");
                    meshPtr->material = material;
                }
            }
            break;

        case Instruction::SET_TWO_SIDED:
            {
                const bool t = instruction.arg;
                if (instruction.part.isRoot()) {
                    instruction.arg.verify(instruction.mesh.isAll(), "part = root() requires mesh = all()");
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        partPtr = m_rootArray[p];
                        for (int m = 0; m < partPtr->m_meshArray.size(); ++m) {
                            partPtr->m_meshArray[m]->twoSided = t;
                        }
                    }
                } else if (instruction.part.isAll()) {
                    instruction.arg.verify(instruction.mesh.isAll(), "part = all() requires mesh = all()");
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        partPtr = m_partArray[p];
                        for (int m = 0; m < partPtr->m_meshArray.size(); ++m) {
                            partPtr->m_meshArray[m]->twoSided = t;
                        }
                    }
                } else {
                    meshPtr = mesh(instruction.part, instruction.mesh);
                    instruction.arg.verify(meshPtr != NULL, "Mesh not found in Part.");
                    meshPtr->twoSided = t;
                }
            }
            break;

        case Instruction::SET_CFRAME:
            {
                const CFrame cframe = instruction.arg;
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cframe = cframe;
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->cframe = cframe;
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.arg.verify(partPtr != NULL, "Part not found.");
                    partPtr->cframe = cframe;
                }        
            }
            break;

        case Instruction::TRANSFORM_CFRAME:
            {
                const CFrame cframe = instruction.arg;
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cframe = cframe * m_rootArray[p]->cframe;
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->cframe = cframe * m_partArray[p]->cframe;
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.arg.verify(partPtr != NULL, "Part not found.");
                    partPtr->cframe = cframe * partPtr->cframe;
                }        
            }
            break;


        case Instruction::RENAME_PART:
            instruction.arg.verify(! instruction.part.isAll() && ! instruction.part.isRoot(), 
                "The argument to renamePart() cannot be all() or root()");
            partPtr = part(instruction.part);
            instruction.arg.verify(partPtr != NULL, "Could not find part");
            partPtr->name = instruction.arg.string(); 
            break;


        case Instruction::RENAME_MESH:
            instruction.arg.verify(! instruction.part.isAll() && ! instruction.part.isRoot(), 
                "The arguments to renameMesh() cannot be all() or root()");
            instruction.arg.verify(! instruction.mesh.isAll() && ! instruction.mesh.isRoot(), 
                "The arguments to renameMesh() cannot be all() or root()");
            meshPtr = mesh(instruction.part, instruction.mesh);
            instruction.arg.verify(meshPtr != NULL, "Could not find mesh");
            meshPtr->name = instruction.arg.string(); 
            break;


        default:
            alwaysAssertM(false, "Instruction not implemented");
        }
    }
}


} // namespace
