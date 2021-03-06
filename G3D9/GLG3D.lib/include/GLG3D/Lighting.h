/**
 \file GLG3D/Lighting.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2002-10-05
 \edited  2011-11-10

 Copyright 2000-2012, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Lighting_h
#define G3D_Lighting_h

#include "G3D/platform.h"
#include "GLG3D/Texture.h"
#include "G3D/Color3.h"
#include "G3D/Vector3.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/GLight.h"

namespace G3D {

typedef ReferenceCountedPointer<class Lighting> LightingRef;

/**
   A rich environment-lighting model that contains both global and local sources.

   When loaded from an Any, environment maps specified as only a filename are assumed to be in sRGB space.
 */
class Lighting : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<class Lighting> Ref;

    /** \beta */
    class Specification {
    public:
        Color3                            emissiveScale;
        Texture::Specification            environmentMapTexture;
        float                             environmentMapConstant;
        Array<GLight>                     lightArray;

        Specification() : emissiveScale(Color3::white()), environmentMapConstant(0.0f) {
            environmentMapTexture.filename  = "<white>";
            environmentMapTexture.dimension = Texture::DIM_CUBE_MAP;
            environmentMapTexture.settings  = Texture::Settings::cubeMap();
        }

        Specification(const class Any&);
        Any toAny() const;
    };

private:

    Lighting() : emissiveScale(Color3::white()), environmentMapConstant(1.0f) {}

public:

    /** Multiply this by all emissive values when rendering.  
        Some algorithms (e.g., G3D::ToneMap) scale
        down light intensity to preserve dynamic range.

        \deprecated
    */
    Color3              emissiveScale;

    /** Cube map or sphere map of the surrounding environment (often just the skybox, although 
         it may be rendered per-object). This contains radiance values.*/
    Texture::Ref        environmentMapTexture;

    /** Scale environmentMap values by this; useful for encoding large
        radiance values in an 8-bit map. */
    float               environmentMapConstant;

    Array<GLight>       lightArray;

    /** A screen-space buffer containing per-pixel ambient occlusion.  This is 
        typically rendered per frame, per viewpoint from a cloned copy of Lighting; it is analogous
        to an environment shadow map for diffuse lighting.

        May be NULL.
        
        \see G3D::AmbientOcclusion
     */
    Texture::Ref        ambientOcclusion;

    /**
      G3D::AmbientOcclusion computes ambientOcclusion with an optional guard band.  This value is the distance
      from (0,0) of the ambientOcclusion texture to (0,0) of the screen.
    */
    Vector2int16        ambientOcclusionOffset;

    /** Creates a (dark) environment. */
    static Ref create(const Specification& s = Specification());

    /** Make a copy of this lighting environment (does not clone the environment map) */
    Lighting::Ref clone() const;

    int numShadowCastingLights() const {
        int n = 0;
        for (int i = 0; i < lightArray.size(); ++i) {
            n += lightArray[i].castsShadows ? 1 : 0;
        }
        return n;
    }

    /** Appends onto the array */
    void getNonShadowCastingLights(Array<GLight>& array) const {
        for (int i = 0; i < lightArray.size(); ++i) {
            if (lightArray[i].castsShadows) {
                array.append(lightArray[i]);
            }
        }
    }

    /** Changes the order of lights */
    void removeShadowCastingLights() {
        for (int i = 0; i < lightArray.size(); ++i) {
            if (lightArray[i].castsShadows) {
                lightArray.fastRemove(i);
                --i;
            }
        }
    }
};

}

#endif
