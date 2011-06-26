/*
 @file Lighting.cpp

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 @created 2002-10-05
 @edited  2011-06-08
 */

#include "GLG3D/Lighting.h"
#include "G3D/Matrix3.h"
#include "G3D/splinefunc.h"
#include "G3D/GLight.h"
#include "G3D/Any.h"
#include <sys/timeb.h>
#include <sys/types.h> 

#ifndef _MSC_VER
   #define _timeb timeb
   #define _ftime ftime
#endif
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4305)
#endif

namespace G3D {



Lighting::Specification::Specification(const Any& any) {
    *this = Specification();
    any.verifyName("Lighting::Specification");

    AnyTableReader r(any);

    r.getIfPresent("emissiveScale", emissiveScale);
    Any evt;
    if (r.getIfPresent("environmentMap", evt)) {
        environmentMapConstant = evt.get("constant", 1.0);
        if (evt.containsKey("texture")) {
            Any t = evt["texture"];
            environmentMapTexture = t;
            if (t.type() == Any::STRING) {
                // Cube map defaults
                environmentMapTexture.dimension = Texture::DIM_CUBE_MAP;
                environmentMapTexture.settings = Texture::Settings::cubeMap();
            }
        }
    }

    Any lt;
    if (r.getIfPresent("lightArray", lt)) {
        lt.verifyType(Any::ARRAY);
        lightArray.resize(lt.size());
        for (int i = 0; i < lt.size(); ++i) {
            lightArray[i] = lt[i];
        }
    }

    r.verifyDone();
}


Any Lighting::Specification::toAny() const {
    Any b(Any::TABLE);
    b["constant"]       = environmentMapConstant;
    b["texture"]        = environmentMapTexture;

    Any a(Any::TABLE, "lighting");
    a["emissiveScale"]  = emissiveScale;
    a["environmentMap"] = b; 
    a["lightArray"]     = lightArray;

    return a;
}

Lighting::Ref Lighting::create(const Specification& s) {
    Lighting::Ref L = new Lighting();
    L->lightArray = s.lightArray;
    if (s.environmentMapTexture.filename != "") {
        L->environmentMapTexture = Texture::create(s.environmentMapTexture);
    }
    L->environmentMapConstant = s.environmentMapConstant;
    return L;
}


LightingRef Lighting::clone() const {
    return new Lighting(*this);
}



}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

