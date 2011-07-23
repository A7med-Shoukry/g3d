#include "GLG3D/ArticulatedModel2.h"

namespace G3D {

ArticulatedModel2::Instruction::Identifier::Identifier(const Any& a) {
    a.verifyType(Any::NUMBER, Any::STRING);
    if (a.type() == Any::NUMBER) {
        id = ID(iRound(a.number()));
    } else {
        name = a.string();
    }
}


ArticulatedModel2::Instruction& ArticulatedModel2::Instruction::operator=(const Any& any) {
    any.verifyType(Any::ARRAY);

    part = Identifier();
    mesh = Identifier();
    arg = Any();

    const std::string& instructionName = any.name();

    if (instructionName == "scale") {

        type = SCALE;
        any.verifySize(1);
        arg = any[0];

    } else if (instructionName == "movePivotBy") {

        type = MOVE_PIVOT_BY;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "setPivot") {

        type = SET_PIVOT;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "transformGeometry") {

        type = TRANSFORM_GEOMETRY;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "deleteMesh") {

        type = DELETE_MESH;
        any.verifySize(2);
        part = any[0];
        mesh = any[1];

    } else if (instructionName == "deletePart") {

        type = DELETE_PART;
        any.verifySize(1);
        part = any[0];

    } else if (instructionName == "setMaterial") {

        type = SET_MATERIAL;
        any.verifySize(3);
        part = any[0];
        mesh = any[1];
        arg = any[2];

    } else if (instructionName == "setTwoSided") {

        type = SET_TWO_SIDED;
        any.verifySize(3);
        part = any[0];
        mesh = any[1];
        arg = any[2];

    } else if (instructionName == "mergeAll") {

        type = MERGE_ALL;
        any.verifySize(0);

    } else if (instructionName == "renamePart") {

        type = RENAME_PART;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "renameMesh") {

        type = RENAME_MESH;
        any.verifySize(3);
        part = any[0];
        mesh = any[1];
        arg = any[2];

    } else {

        any.verify(false, std::string("Unknown instruction: \"") + instructionName + "\"");
    }
    return *this;
}

} // namespace
