/**
 @file Lighting.h

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 @created 2002-10-05
 @edited  2010-09-06

 Copyright 2000-2010, Morgan McGuire.
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

#define BROWN_UNIVERSITY_LATITUDE 41.7333f
#define BROWN_UNIVERSITY_LONGITUDE 71.4333f

#define WILLIAMS_COLLEGE_LATITUDE 42.71f
#define WILLIAMS_COLLEGE_LONGITUDE -73.20f

/* Definition of a sidereal day */
#define SIDEREAL_DAY ((23*HOUR)+(56*MINUTE)+(4.071f*SECOND))

typedef ReferenceCountedPointer<class Lighting> LightingRef;
typedef ReferenceCountedPointer<class Sky> SkyRef;
/**
 Provides a reasonable (but not 100% physically correct) set of lighting 
 parameters based on the time of day.  See also G3D::Lighting, which describes
 a rich lighting environment.

 \deprecated
 */
class SkyParameters {
public:
    /** Multiply this by all emissive values when rendering.  
        Some algorithms (e.g. contrib/ArticulatedModel/ToneMap) scale
        down light intensity to preserve dynamic range.*/
    Color3                  emissiveScale;

    /** Modulate sky box color */
    Color3                  skyAmbient;

    /**
     Use this for objects that do not receive directional lighting
     (e.g. billboards).
     */
    Color3                  diffuseAmbient;

    /**
     Directional light color.
     */
    Color3                  lightColor;
    Color3                  ambient;

    /** Only one light source, the sun or moon, is active at a given time. */
    Vector3                 lightDirection;
    enum {SUN, MOON}        source;

    /** Using physically correct parameters.  When false, the sun and moon
        travel in a perfectly east-west arc where +x = east and -x = west. */
    bool                    physicallyCorrect;

    /** The vector <B>to</B> the sun */
    Vector3		            trueSunPosition;
    Vector3                 sunPosition;

    /** The vector <B>to</B> the moon */
    Vector3		            trueMoonPosition;
    Vector3                 moonPosition;
	double			        moonPhase;	

    /** The coordinate frame and vector related to the starfield */
    CoordinateFrame	        starFrame;
	CoordinateFrame		    trueStarFrame;
    Vector3		            starVec;

    /** Geographic position */
    float                   geoLatitude;
	
    SkyParameters();

    /**
     Sets light parameters for the sun/moon based on the
     specified time since midnight, as well as geographic 
     latitude for starfield orientation (positive for north 
     of the equator and negative for south) and geographic 
     longitude for sun positioning (postive for east of 
     Greenwich, and negative for west). The latitude and 
     longitude is set by default to that of Williamstown, MA, 
     USA.
     */
     SkyParameters(
	     const GameTime     _time,
	     bool 	            _physicallyCorrect = true,
	     float              _latitude = WILLIAMS_COLLEGE_LATITUDE);

    void setTime(const GameTime _time);
	void setLatitude(float _latitude);

    /**
     Returns a directional light composed from the light direction
     and color.
     */
    GLight directionalLight() const;

};


// TODO: Remove
/** @deprecated */
typedef SkyParameters SkyParameters;

/**
   A rich environment lighting model that contains both global and local sources.

   Note: This class will change substantially in G3D 9.00.
 */
class Lighting : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<class Lighting> Ref;

    /** \beta */
    class Specification {
    public:
        Color3                            emissiveScale;
        Texture::Specification            environmentMap;
        float                             environmentMapScale;
        Array<GLight>                     lightArray;
        Specification() : emissiveScale(Color3::white()), environmentMapScale(1.0f) {}
        Specification(const class Any&);
        operator Any() const;
    };

private:

    Lighting() : emissiveScale(Color3::white()), environmentMapScale(1.0f) {}

public:

    /** \deprecated */
    static Lighting::Ref fromSky(const SkyRef& sky, const SkyParameters&, const Color3& groundColor);

    /** Multiply this by all emissive values when rendering.  
        Some algorithms (e.g., G3D::ToneMap) scale
        down light intensity to preserve dynamic range.

        \deprecated
    */
    Color3              emissiveScale;

    /** Cube map or sphere map of the surrounding environment (often just the skybox, although 
         it may be rendered per-object). This contains radiance values.*/
    Texture::Ref        environmentMap;

    /** Scale environmentMap values by this; useful for encoding large radiance values in an 8-bit map. */
    float               environmentMapScale;

    Array<GLight>       lightArray;

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
