#include "GLG3D/Texture.h"

namespace G3D {

Texture::Visualization::Visualization(Channels c, float g, float mn, float mx) : 
    channels(c), documentGamma(g), min(mn), max(mx), invertIntensity(false) {
}


Texture::Visualization::Visualization(const Any& a) {
    *this = Visualization();
//    AnyTableReader
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
        (channels != RGB) ||
        (documentGamma != 2.2f) ||
        (min != 0.0f) ||
        (max != 1.0f) ||
        invertIntensity;
}

}
