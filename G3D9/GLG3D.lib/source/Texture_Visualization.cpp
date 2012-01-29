#include "GLG3D/Texture.h"

namespace G3D {

Texture::Visualization::Visualization(Channels c, float g, float mn, float mx, bool isCube) : 
    channels(c), documentGamma(g), min(mn), max(mx), invertIntensity(false), isCubemap(isCube) {
}

const static std::string name[] = 
    {"RGB", "R", "G", "B", "RasL", "GasL", "BasL", "AasL", "MeanRGBasL", "Luminance", ""};

static std::string channelsToString(const Texture::Visualization::Channels& c) {
    return name[c];
}

static Texture::Visualization::Channels toChannels(const std::string& s) {

    for (int i = 0; ! name[i].empty(); ++i) {
        if (s == name[i]) {
            return Texture::Visualization::Channels(i);
        }
    }

    return Texture::Visualization::RGB;
}


Any Texture::Visualization::toAny() const {
    Any a(Any::TABLE, "Texture::Visualization");
    a["channels"] = channelsToString(channels);
    a["documentGamma"] = documentGamma;
    a["min"] = min;
    a["max"] = max;
    a["invertIntensity"] = invertIntensity;
	a["isCubemap"] = isCubemap;
    return a;
}


Texture::Visualization::Visualization(const Any& a) {
    *this = Visualization();
    if (a.type() == Any::ARRAY) {
        if (a.nameEquals("bumpInAlpha")) {
            *this = bumpInAlpha();
        } else if (a.nameEquals("defaults")) {
            *this = defaults();
        } else if (a.nameEquals("linearRGB")) {
            *this = linearRGB();
        } else if (a.nameEquals("depthBuffer")) {
            *this = depthBuffer();
        } else if (a.nameEquals("packedUnitVector")) {
            *this = packedUnitVector();
        } else if (a.nameEquals("radiance")) {
            *this = radiance();
        } else if (a.nameEquals("reflectivity")) {
            *this = reflectivity();
        } else if (a.nameEquals("sRGB")) {
            *this = sRGB();
        } else if (a.nameEquals("unitVector")) {
            *this = unitVector();
        } else {
            a.verify(false, "Unrecognized Visualization factory method");
        }
    } else {
        a.verifyName("Texture::Visualization", "Visualization");

        AnyTableReader r(a);
        std::string c;

        if (r.getIfPresent("channels", c)) {
            channels = toChannels(c);
        }

        r.getIfPresent("documentGamma", documentGamma);
        r.getIfPresent("invertIntensity", invertIntensity);
        r.getIfPresent("max", max);
        r.getIfPresent("min", min);
		r.getIfPresent("isCubemap", isCubemap);

        r.verifyDone();
    }
}


const Texture::Visualization& Texture::Visualization::sRGB() {
    static const Texture::Visualization s(RGB, 2.2f, 0.0f, 1.0f);
    return s;
}


const Texture::Visualization& Texture::Visualization::unitVector() {
    static const Texture::Visualization s(RGB, 1.0f, -1.0f, 1.0f);
    return s;
}


const Texture::Visualization& Texture::Visualization::depthBuffer() {
    static const Texture::Visualization s(RasL, 9.0f, 0.2f, 1.0f);
    return s;
}


const Texture::Visualization& Texture::Visualization::bumpInAlpha() {
    static const Texture::Visualization s(AasL, 1.0f, 0.0f, 1.0f);
    return s;
}


const Texture::Visualization& Texture::Visualization::defaults() {
    static const Texture::Visualization s;
    return s;
}


bool Texture::Visualization::needsShader() const {
    return 
        true || // TODO: currently set to true because the non-shader path of GuiTextureBox is broken
        (channels != RGB) ||
        (documentGamma != 2.2f) ||
        (min != 0.0f) ||
        (max != 1.0f) ||
        invertIntensity ||
		isCubemap;
}

}
