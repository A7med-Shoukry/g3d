#include "GLG3D/ArticulatedModel2.h"

namespace G3D {
    

void ArticulatedModel2::preprocess(const Array<Instruction>& program) {
    for (int i = 0; i < program.size(); ++i) {
        const Instruction& instruction = program[i];

        Part* partPtr = NULL;
        Mesh* meshPtr = NULL;

        switch (instruction.type) {
        case Instruction::SCALE:
            {
                // Scale every pivot translation and every vertex position by the scale factor
                float scaleFactor = instruction.arg;
                ScaleTransformCallback transform(scaleFactor);
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

        case Instruction::TRANSFORM_GEOMETRY:
            {
                const Matrix4 transform = instruction.arg;
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->transformGeometry(transform);
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->transformGeometry(transform);
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.arg.verify(partPtr != NULL, "Part not found.");
                    partPtr->transformGeometry(transform);
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

        case Instruction::ADD:
            partPtr = NULL;
            if (! instruction.part.isNone()) {
                partPtr = part(instruction.part);
                instruction.source.verify(partPtr != NULL, "Unrecognized parent part");
            }
            {
                // Load the child part
                ArticulatedModel2::Ref m2 = ArticulatedModel2::create(Specification(instruction.arg));

                // Update part table, mesh table, and overwrite IDs
                for (int p = 0; p < m2->m_partArray.size(); ++p) {

                    Part* part = m2->m_partArray[p];
                    const_cast<ID&>(part->id) = createID();
                    m_partTable.set(part->id, part);

                    for (int m = 0; m < part->m_meshArray.size(); ++m) {
                        Mesh* mesh = part->m_meshArray[m];
                        const_cast<ID&>(mesh->id) = createID();
                        m_meshTable.set(mesh->id, mesh);
                    }
                }

                // Steal all elements of the child and add them to this
                if (partPtr == NULL) {
                    // Add as roots
                    m_rootArray.append(m2->m_rootArray);
                } else {
                    // Reparent
                    partPtr->m_child.append(m2->m_rootArray);
                    for (int p = 0; p < m2->m_partArray.size(); ++p) {
                        if (m2->m_partArray[p]->isRoot()) {
                            m2->m_partArray[p]->m_parent = partPtr;
                        }
                    }
                }
                m_partArray.append(m2->m_partArray);

                // Allow m2 to be garbage collected
            }
            break;

        default:
            alwaysAssertM(false, "Instruction not implemented");
        }
    }
}


void ArticulatedModel2::Part::transformGeometry(const Matrix4& xform) {
    CPUVertexArray::Vertex* vertex = cpuVertexArray.vertex.getCArray();
    const int N = cpuVertexArray.size();
    for (int i = 0; i < N; ++i) {
        vertex->position = xform.homoMul(vertex->position, 1.0f);
        vertex->tangent  = Vector4::nan();
        vertex->normal   = Vector3::nan();
        ++vertex;
    }
}


} // namespace
