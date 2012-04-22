/**
 \file    BumpMap.cpp
 \author  Morgan McGuire, http://graphics.cs.williams.edu
 \created 2009-03-25
 \edited  2011-06-28
*/
#include "GLG3D/BumpMap.h"
#include "G3D/Any.h"
#include "G3D/SpeedLoad.h"

namespace G3D {

bool BumpMap::Specification::operator==(const Specification& other) const {
    return (texture == other.texture) && (settings == other.settings);
}


BumpMap::Specification::Specification(const Any& any) {
    if (any.type() == Any::STRING) {
        // Treat as a filename
        texture.filename = any.resolveStringAsFilename();
        texture.preprocess = Texture::Preprocess::normalMap();
    } else {
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const std::string& key = toLower(it->key);
            if (key == "texture") {
                texture = it->value;
                if (it->value.type() == Any::STRING) {
                    // Set bump map defaults
                    texture.preprocess = Texture::Preprocess::normalMap();
                }
            } else if (key == "settings") {
                settings = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);        
            }
        }
    }
}


BumpMap::Ref BumpMap::speedCreate(BinaryInput& b) {
    BumpMap::Ref bump = new BumpMap();

    SpeedLoad::readHeader(b, "BumpMap");

    bump->m_normalBump = MapComponent<Image4>::speedCreate(b);
    bump->m_settings.deserialize(b);
    
    return bump;
}

    
void BumpMap::speedSerialize(BinaryOutput& b) const {
    SpeedLoad::writeHeader(b, "BumpMap");
    m_normalBump->speedSerialize(b);
    m_settings.serialize(b);
}


BumpMap::BumpMap(const MapComponent<Image4>::Ref& normalBump, const Settings& settings) : 
    m_normalBump(normalBump), m_settings(settings) {
}


BumpMap::Ref BumpMap::create(const MapComponent<Image4>::Ref& normalBump, const Settings& settings) {
    return new BumpMap(normalBump, settings);
}


BumpMap::Ref BumpMap::create(const Specification& spec) {
    return BumpMap::create(MapComponent<Image4>::create(NULL, Texture::create(spec.texture)), spec.settings);    
}


static bool hasTexture(const Component4& c) {
    return 
        (c.factors() == Component4::MAP) ||
        (c.factors() == Component4::MAP_TIMES_CONSTANT);
}


bool BumpMap::similarTo(const BumpMap::Ref& other) const {
    return
        (m_settings.iterations == other->m_settings.iterations) &&
        (hasTexture(m_normalBump) == hasTexture(other->m_normalBump));
}

///////////////////////////////////////////////////////////

BumpMap::Settings::Settings(const Any& any) {
    *this = Settings();
    any.verifyName("BumpMap::Settings");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "iterations") {
            iterations = iMax(0, iRound(it->value.number()));
        } else if (key == "scale") {
            scale = it->value;
        } else if (key == "bias") {
            bias = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}


Any BumpMap::Settings::toAny() const {
    Any any(Any::TABLE, "BumpMap::Settings");
    any["scale"] = scale;
    any["bias"] = bias;
    any["iterations"] = iterations;
    return any;
}


bool BumpMap::Settings::operator==(const Settings& other) const {
    return 
        (scale == other.scale) &&
        (bias == other.bias) &&
        (iterations == other.iterations);

}


ImageBuffer::Ref BumpMap::computeNormalMap
(int                 width,
 int                 height,
 int                 channels,
 const unorm8*       src,
 const BumpMapPreprocess& preprocess) {

    ImageBuffer::Ref normal = ImageBuffer::create(width, height, ImageFormat::RGBA8());
    float whiteHeightInPixels = preprocess.zExtentPixels;
    bool lowPassBump          = preprocess.lowPassFilter;
    bool scaleHeightByNz      = preprocess.scaleZByNz;
    
    if (whiteHeightInPixels < 0.0f) {
        // Default setting scales so that a gradient ramp
        // over the whole image becomes a 45-degree angle
        
        // Account for potentially non-square aspect ratios
        whiteHeightInPixels = max(width, height) * -whiteHeightInPixels;
    }

    debugAssert(whiteHeightInPixels >= 0);

    const int w = width;
    const int h = height;
    const int stride = channels;
    
    const unorm8* const B = src;
    Color4unorm8* const N = static_cast<Color4unorm8*>(normal->buffer());

    // 1/s for the scale factor that each ELEVATION should be multiplied by.
    // We avoid actually multiplying by this and instead just divide it out of z.
    float elevationInvScale = 255.0f / whiteHeightInPixels;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Index into normal map pixel
            int i = x + y * w;

            // Index into bump map *byte*
            int j = stride * i;

            Vector3 delta;

            // Get a value from B (with wrapping lookup) relative to (x, y)
            // and divide by 255
#define ELEVATION(DX, DY)  ((int)(B[(((DX + x + w) % w) +               \
                                     ((DY + y + h) % h) * w) * stride].bits()))


            // Sobel filter to compute the normal.  
            //
            // Y Filter (X filter is the transpose)
            //  [ -1 -2 -1 ]
            //  [  0  0  0 ]
            //  [  1  2  1 ]

            // Write the Y value directly into the x-component so we don't have
            // to explicitly compute a cross product at the end.  Does not 
            // go out of bounds because the above is computed mod (width, height)
            delta.y = -( ELEVATION(-1, -1) * 1 +  ELEVATION( 0, -1) * 2 +  ELEVATION( 1, -1) * 1 +
                        -ELEVATION(-1,  1) * 1 + -ELEVATION( 0,  1) * 2 + -ELEVATION( 1,  1) * 1);

            delta.x = -(-ELEVATION(-1, -1) * 1 + ELEVATION( 1, -1) * 1 + 
                        -ELEVATION(-1,  0) * 2 + ELEVATION( 1,  0) * 2 + 
                        -ELEVATION(-1,  1) * 1 + ELEVATION( 1,  1) * 1);

            // The scale of each filter row is 4, the filter width is two pixels,
            // and the "normal" range is 0-255.
            delta.z = 4 * 2 * elevationInvScale;

            // Delta is now scaled in pixels; normalize 
            delta = delta.direction();

            // Copy over the bump value into the alpha channel.
            float H = B[j];

            if (lowPassBump) {
                H = (ELEVATION(-1, -1) + ELEVATION( 0, -1) + ELEVATION(1, -1) +
                     ELEVATION(-1,  0) + ELEVATION( 0,  0) + ELEVATION(1,  0) +
                     ELEVATION(-1,  1) + ELEVATION( 0,  1) + ELEVATION(1,  1)) / (255.0f * 9.0f);
            }
#           undef ELEVATION

            if (scaleHeightByNz) {
                // delta.z can't possibly be negative, so we avoid actually
                // computing the absolute value.
                H *= delta.z;
            }

            N[i].a = unorm8(H);

            // Pack into byte range
            delta = delta * 0.5f + Vector3(0.5f, 0.5f, 0.5f);
            N[i].r = unorm8(delta.x);
            N[i].g = unorm8(delta.y);
            N[i].b = unorm8(delta.z);
        }
    }

    return normal;
}

} // G3D
