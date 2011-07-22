#include "ArticulatedModel2.h"

ArticulatedModel2::CleanGeometrySettings::CleanGeometrySettings(const Any& a) {
    *this = CleanGeometrySettings();
    AnyTableReader r(a);
    r.getIfPresent("forceVertexMerging", forceVertexMerging);
    r.getIfPresent("maxNormalWeldAngle", maxNormalWeldAngle);
    r.getIfPresent("maxSmoothAngle",     maxSmoothAngle);
    r.verifyDone();
}


Any ArticulatedModel2::CleanGeometrySettings::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel2::CleanGeometrySettings");
    a["forceVertexMerging"] = forceVertexMerging;
    a["maxNormalWeldAngle"] = maxNormalWeldAngle;
    a["maxSmoothAngle"]     = maxSmoothAngle;
    return a;
}


//////////////////////////////////////////////////////////////////////

ArticulatedModel2::Specification::Specification(const Any& a) {
    *this = Specification();
    AnyTableReader r(a);
    r.getIfPresent("filename",                  filename);
    r.getIfPresent("xform",                     xform);
    r.getIfPresent("stripMaterials",            stripMaterials);
    r.getIfPresent("mergeMeshesByMaterial",     mergeMeshesByMaterial);
    r.getIfPresent("cleanGeometrySettings",     cleanGeometrySettings);
    r.verifyDone();
}


Any ArticulatedModel2::Specification::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel2::Specification");
    a["filename"]                  = filename;
    a["xform"]                     = xform;
    a["stripMaterials"]            = stripMaterials;
    a["mergeMeshesByMaterial"]     = mergeMeshesByMaterial;
    a["cleanGeometrySettings"]     = cleanGeometrySettings;
    return a;
}

//////////////////////////////////////////////////////////////////////

ArticulatedModel2::PoseSpline::PoseSpline() {}


ArticulatedModel2::PoseSpline::PoseSpline(const Any& any) {
    any.verifyName("ArticulatedModel2::PoseSpline");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        partSpline.getCreate(it->key) = it->value;
    }
}

 
void ArticulatedModel2::PoseSpline::get(float t, ArticulatedModel2::Pose& pose) {
    for (SplineTable::Iterator it = partSpline.begin(); it.isValid(); ++it) {
        if (it->value.control.size() > 0) {
            pose.cframe.set(it->key, it->value.evaluate(t));
        }
    }
}