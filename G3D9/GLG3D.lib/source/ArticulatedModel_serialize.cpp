/**
 \file GLG3D/source/ArticulatedModel_zerialize.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-18
 \edited  2011-07-22
 
 Copyright 2000-2011, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel.h"

namespace G3D {

ArticulatedModel::CleanGeometrySettings::CleanGeometrySettings(const Any& a) {
    *this = CleanGeometrySettings();
    AnyTableReader r(a);
    r.getIfPresent("forceVertexMerging", forceVertexMerging);
    r.getIfPresent("maxNormalWeldAngle", maxNormalWeldAngle);
    r.getIfPresent("maxSmoothAngle",     maxSmoothAngle);
    r.verifyDone();
}


Any ArticulatedModel::CleanGeometrySettings::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::CleanGeometrySettings");
    a["forceVertexMerging"] = forceVertexMerging;
    a["maxNormalWeldAngle"] = maxNormalWeldAngle;
    a["maxSmoothAngle"]     = maxSmoothAngle;
    return a;
}


//////////////////////////////////////////////////////////////////////

ArticulatedModel::Specification::Specification(const Any& a) {
    *this = Specification();
    AnyTableReader r(a);
    Any f;
    r.getIfPresent("filename",                  f);
    filename = f.resolveStringAsFilename();

    r.getIfPresent("stripMaterials",            stripMaterials);
    r.getIfPresent("mergeMeshesByMaterial",     mergeMeshesByMaterial);
    r.getIfPresent("cleanGeometrySettings",     cleanGeometrySettings);
    r.getIfPresent("scale",                     scale);
    r.getIfPresent("preprocess",                preprocess);

    r.verifyDone();
}


Any ArticulatedModel::Specification::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::Specification");
    a["filename"]                  = filename;
    a["stripMaterials"]            = stripMaterials;
    a["mergeMeshesByMaterial"]     = mergeMeshesByMaterial;
    a["cleanGeometrySettings"]     = cleanGeometrySettings;
    a["scale"]                     = scale;

    if (preprocess.size() > 0) {
        a["preprocess"] = Any(preprocess, "preprocess");
    }
    return a;
}

//////////////////////////////////////////////////////////////////////

ArticulatedModel::PoseSpline::PoseSpline() : castsShadows(true) {}


ArticulatedModel::PoseSpline::PoseSpline(const Any& any) : castsShadows(true) {
    any.verifyName("ArticulatedModel::PoseSpline");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        if (it->key == "castsShadows") {
            castsShadows = it->value;
        } else {
            partSpline.getCreate(it->key) = it->value;
        }
    }
}

 
void ArticulatedModel::PoseSpline::get(float t, ArticulatedModel::Pose& pose) {
    for (SplineTable::Iterator it = partSpline.begin(); it.isValid(); ++it) {
        if (it->value.control.size() > 0) {
            pose.cframe.set(it->key, it->value.evaluate(t));
        }
    }

    pose.castsShadows = castsShadows;
}

///////////////////////////////////////////////////////////////////////

ArticulatedModel::Instruction::Identifier::Identifier(const Any& a) {
    switch (a.type()) {
    case Any::NUMBER:
        id = ID(iRound(a.number()));
        a.verify(id >= 0, "Illegal ID");
        break;

    case Any::STRING:
        name = a.string();
        break;

    case Any::ARRAY:
        a.verifySize(0);
        if (a.name() == "root") {
            *this = root();
        } else if (a.name() == "all") {
            *this = all();
        } else {
            a.verify(false, "Illegal function call: " + a.name());
        }
        break;

    default:
        a.verify(false, "Expected a name, integer ID, root(), or all()");
    }
}

Any ArticulatedModel::Instruction::Identifier::toAny() const {
    if (isAll()) {
        return Any(Any::ARRAY, "all");
    } else if (isRoot()) {
        return Any(Any::ARRAY, "root");
    } else if (id.initialized()) {
        return Any(int(id));
    } else {
        return Any(name);
    }
}

///////////////////////////////////////////////////////////////////////

Any ArticulatedModel::Instruction::toAny() const {
    return source;
}

ArticulatedModel::Instruction::Instruction(const Any& any) {
    any.verifyType(Any::ARRAY);

    source = any;
    part = Identifier();
    mesh = Identifier();
    arg = Any();

    const std::string& instructionName = any.name();

    if (instructionName == "scale") {

        type = SCALE;
        any.verifySize(1);
        arg = any[0];

    } else if (instructionName == "moveCenterToOrigin") {

        type = MOVE_CENTER_TO_ORIGIN;
        any.verifySize(0);

    } else if (instructionName == "moveBaseToOrigin") {

        type = MOVE_BASE_TO_ORIGIN;
        any.verifySize(0);

    } else if (instructionName == "setCFrame") {

        type = SET_CFRAME;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "transformCFrame") {

        type = TRANSFORM_CFRAME;
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

    } else if (instructionName == "add") {

        type = ADD;
        mesh = Identifier::none();
        if (any.size() == 2) {
            any.verifySize(2);
            part = any[0];
            arg = any[1];
        } else {
            any.verifySize(1);
            part = Identifier::none();
            arg = any[0];
        }

    } else {

        any.verify(false, std::string("Unknown instruction: \"") + instructionName + "\"");

    }
}

} // namespace G3D
