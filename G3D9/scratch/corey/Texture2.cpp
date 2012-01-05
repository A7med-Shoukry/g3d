/**
  \file GLG3D/Texture2.h
 
  Copyright 2000-2012, Morgan McGuire.
  All rights reserved.
 */

#include "G3D/FileSystem.h"
#include "GLG3D/GLCaps.h"
#include "Texture2.h"

namespace G3D {


//// Texture::Specification implementation

Any Texture2::Specification::toAny() const {
    Any a = Any(Any::TABLE, "Texture2::Specification");
    a["filename"] = filename;
    a["desiredFormat"] = desiredFormat ? desiredFormat->name() : "AUTO";
    a["dimension"] = Texture2::toString(dimension);
    a["settings"]  = settings;
    //a["preprocess"] = preprocess;  disabled for Texture2 re-write
    //a["visualization"] = visualization; disabled for Texture2 re-write

    return a;
}


void Texture2::Specification::serialize(BinaryOutput& b) const {
    toAny().serialize(b);
}


void Texture2::Specification::deserialize(BinaryInput& b) {
    Any a;
    a.deserialize(b);
    *this = a;
}


bool Texture2::Specification::operator==(const Specification& other) const {
    return 
        (filename == other.filename) &&
        (desiredFormat == other.desiredFormat) &&
        (dimension == other.dimension) &&
        (settings == other.settings);
        //(preprocess == other.preprocess) &&
        //(visualization == other.visualization);
}


Texture2::Specification::Specification(const Any& any) {
    *this = Specification();

    if (any.type() == Any::STRING) {
        filename = any.resolveStringAsFilename();
        if (FilePath::containsWildcards(filename)) {
            // Assume this is a cube map
            settings = Texture2::Settings::cubeMap();
        }
    } else {
        any.verifyNameBeginsWith("Texture2::Specification");
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const std::string& key = it->key;
            if (key == "filename") {
                filename = it->value.resolveStringAsFilename();
            } else if (key == "desiredFormat") {
                desiredFormat = ImageFormat::fromString(it->value.string());
            } else if (key == "dimension") {
                dimension = toDimension(it->value);
            } else if (key == "settings") {
                settings = it->value;
            //} else if (key == "preprocess") {
                //preprocess = it->value;
            //} else if (key == "visualization") {
                //visualization = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
        if (! any.containsKey("settings") && FilePath::containsWildcards(filename)) {
            // Assume this is a cube map
            settings = Texture2::Settings::cubeMap();
        }
    }
}

//// Texture::Settings implementation

const char* Texture2::toString(DepthReadMode m) {
    switch (m) {
    case DEPTH_NORMAL: return "DEPTH_NORMAL";
    case DEPTH_LEQUAL: return "DEPTH_LEQUAL";
    case DEPTH_GEQUAL: return "DEPTH_GEQUAL";
    default:
        return "ERROR";
    }
}


Texture2::Dimension Texture2::toDimension(const std::string& s) {
    // The NPOT and RECT versions are for backward compatibility on reading older .any files
    if (s == "DIM_2D" || s == "DIM_2D_NPOT" || s == "DIM_2D_RECT") {
        return DIM_2D;
    } else if (s == "DIM_3D" || s == "DIM_3D_NPOT") {
        return DIM_3D;
    } else if (s == "DIM_CUBE" || s == "DIM_CUBE_MAP" || s == "DIM_CUBE_MAP_NPOT") {
        return DIM_CUBE;
    } else {
        debugAssertM(false, "Unrecognized dimension");
        return DIM_2D;
    }
}


const char* Texture2::toString(Dimension d) {
    switch (d) {
    case DIM_2D:    return "DIM_2D";
    case DIM_3D:    return "DIM_3D";
    case DIM_CUBE:  return "DIM_CUBE";
    default:
        debugAssertM(false, "Unrecognized dimension");
        return "ERROR";
    }
}


Texture2::DepthReadMode Texture2::toDepthReadMode(const std::string& s) {
    if (s == "DEPTH_NORMAL") {
        return DEPTH_NORMAL;
    } else if (s == "DEPTH_LEQUAL") {
        return DEPTH_LEQUAL;
    } else if (s == "DEPTH_GEQUAL") {
        return DEPTH_GEQUAL;
    } else {
        debugAssertM(false, "Unrecognized depth read mode");
        return DEPTH_NORMAL;
    }
}


const char* Texture2::toString(InterpolateMode m) {
    switch (m) {
    case TRILINEAR_MIPMAP:   return "TRILINEAR_MIPMAP";
    case BILINEAR_MIPMAP:    return "BILINEAR_MIPMAP";
    case NEAREST_MIPMAP:     return "NEAREST_MIPMAP";
    case BILINEAR_NO_MIPMAP: return "BILINEAR_NO_MIPMAP";
    case NEAREST_NO_MIPMAP:  return "NEAREST_NO_MIPMAP";
    default:
        return "ERROR";
    }
}


Texture2::InterpolateMode Texture2::toInterpolateMode(const std::string& s) {
    if (s == "TRILINEAR_MIPMAP") {
        return TRILINEAR_MIPMAP;
    } else if (s == "BILINEAR_MIPMAP") {
        return BILINEAR_MIPMAP;
    } else if (s == "NEAREST_MIPMAP") {
        return NEAREST_MIPMAP;
    } else if (s == "BILINEAR_NO_MIPMAP") {
        return BILINEAR_NO_MIPMAP;
    } else if (s == "NEAREST_NO_MIPMAP") {
        return NEAREST_NO_MIPMAP;
    } else {
        debugAssertM(false, "Unrecognized interpolate mode");
        return TRILINEAR_MIPMAP;
    }
}


Any Texture2::Settings::toAny() const {
    Any a(Any::TABLE, "Texture2::Settings");
    a["interpolateMode"] = toString(interpolateMode);
    a["wrapMode"]        = wrapMode.toAny();
    a["maxAnisotropy"]   = maxAnisotropy;
    a["autoMipMap"]      = autoMipMap;
    a["maxMipMap"]       = maxMipMap;
    a["minMipMap"]       = minMipMap;
    return a;
}


Texture2::Settings::Settings(const Any& any) {
    *this = Settings::defaults();
    any.verifyNameBeginsWith("Texture2::Settings");
    if (any.type() == Any::TABLE) {
        AnyTableReader r(any);

        r.getIfPresent("autoMipMap", autoMipMap);
        Any temp;
        if (r.getIfPresent("depthReadMode", temp)) {
            depthReadMode = toDepthReadMode(temp);
        }
        if (r.getIfPresent("interpolateMode", temp)) {
            interpolateMode = toInterpolateMode(temp);
        }
        r.getIfPresent("maxAnisotropy", maxAnisotropy);
        r.getIfPresent("maxMipMap", maxMipMap);
        r.getIfPresent("minMipMap", minMipMap);
        r.getIfPresent("wrapMode", wrapMode);
        r.verifyDone();
    } else {
        any.verifySize(0);
        const std::string& n = any.name();
        if (n == "Texture2::Settings::defaults") {
            // Done!
        } else if (n == "Texture2::Settings::buffer") {
            *this = Texture2::Settings::buffer();
        } else if (n == "Texture2::Settings::cubeMap") {
            *this = Texture2::Settings::cubeMap();
        } else if (n == "Texture2::Settings::shadow") {
            *this = Texture2::Settings::shadow();
        } else if (n == "Texture2::Settings::video") {
            *this = Texture2::Settings::video();
        } else {
            any.verify(false, "Unrecognized name for Texture2::Settings constructor or factory method.");
        }
    }
}

Texture2::Settings::Settings() : 
    interpolateMode(TRILINEAR_MIPMAP),
    wrapMode(WrapMode::TILE),
    depthReadMode(DEPTH_NORMAL),
    maxAnisotropy(4.0),
    autoMipMap(true),
    maxMipMap(1000),
    minMipMap(-1000) {
}


const Texture2::Settings& Texture2::Settings::defaults() {
    static Settings param;
    return param;
}


const Texture2::Settings& Texture2::Settings::video() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = BILINEAR_NO_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = false;
    }

    return param;
}


const Texture2::Settings& Texture2::Settings::buffer() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = NEAREST_NO_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = false;
    }

    return param;
}


const Texture2::Settings& Texture2::Settings::cubeMap() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = BILINEAR_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = true;
    }

    return param;
}


const Texture2::Settings& Texture2::Settings::shadow() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        if (GLCaps::enumVendor() == GLCaps::ATI) {
            // ATI cards do not implement PCF for shadow maps
            param.interpolateMode = NEAREST_NO_MIPMAP;
        } else {
            param.interpolateMode = BILINEAR_NO_MIPMAP;
        }
        param.wrapMode      = WrapMode::ZERO;
        param.depthReadMode = DEPTH_LEQUAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap    = false;
    }

    return param;
}


size_t Texture2::Settings::hashCode() const {
    return 
        (uint32)interpolateMode + 
        16 * (uint32)wrapMode + 
        256 * (uint32)depthReadMode + 
        (autoMipMap ? 512 : 0) +
        (uint32)(1024 * maxAnisotropy) +
        (minMipMap ^ (maxMipMap << 16));
}

bool Texture2::Settings::operator==(const Settings& other) const {
    return 
        (interpolateMode == other.interpolateMode) &&
        (wrapMode == other.wrapMode) &&
        (depthReadMode == other.depthReadMode) &&
        (maxAnisotropy == other.maxAnisotropy) &&
        (autoMipMap == other.autoMipMap) &&
        (maxMipMap == other.maxMipMap) &&
        (minMipMap == other.minMipMap);
}

bool Texture2::Settings::equalsIgnoringMipMap(const Settings& other) const {
    return 
        (interpolateMode == other.interpolateMode) &&
        (wrapMode == other.wrapMode) &&
        (depthReadMode == other.depthReadMode);
}

} // namespace G3D
