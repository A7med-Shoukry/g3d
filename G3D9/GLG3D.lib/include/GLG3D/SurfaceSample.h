#ifndef GLG3D_SurfaceSample_h
#define GLG3D_SurfaceSample_h

#include "GLG3D/Tri.h"
#include "GLG3D/Material.h"

namespace G3D {

/** A sample of a surface at a point, describing the material
    and geometric properties needed for shading.
    
    The current implementation does not actually sample the
    bump/normal map */
class SurfaceSample {
public:

    /** Location after bump map is applied. */
    Point3     shadingLocation;
    
    /** Normal after normal map is applied. */
    Vector3    shadingNormal;

    /** Smoothed (e.g., interpolated vertex normals) normal for curved surfaces. */
    Vector3    interpolatedNormal;

    /** Location before bump map is applied. */
    Vector3    geometricLocation;

    /** Normal to the true surface (e.g., containing triangle face)
        at the shading location. */
    Vector3    geometricNormal;
    
    /** Approximately orthogonal to interpolatedNormal. */
    Vector3    interpolatedTangent;
    
    /** Approximately orthogonal to interpolatedNormal and interpolatedTangent. */
    Vector3    interpolatedTangent2;

    /** Texture coordinate */
    Vector2    texCoord;

    /** Screen space derivative of the texture coordinate.  \beta Currently unused. */
    Vector2    dTexCoorddX;

    /** Screen space derivative of the texture coordinate.  \beta Currently unused. */
    Vector2    dTexCoorddY;

    /** "alpha" value */
    float      coverage;

    /** Sampled from BSDF */
    Color3     lambertianReflect;

    /** Sampled from BSDF */
    Color3     glossyReflect;

    /** Sampled from BSDF */
    float      glossyExponent;

    /** Sampled from BSDF */
    Color3     transmit;

    /** Sampled from BSDF */
    Color3     extinctionReflect;

    /** Sampled from BSDF */
    Color3     extinctionTransmit;

    /** Sampled from the emission map. */
    Power3     emit;

    SurfaceSample() : coverage(0.0f), glossyExponent(0.0f) {}

    SurfaceSample(const Tri::Intersector& intersector);
    
    /** Samples just the emission using the existing texCoord, leaving
        other fields unchanged. */
    void sampleEmit(const Component3& emitMap);

    /** Samples just the BSDF using the existing texCoord, leaving
        non-BSDF fields unchanged. */
    void sampleBSDF(const SuperBSDF::Ref& bsdf);
    
    /** Samples the bump map fields, using the existing texCoord,
        interpolatedNormal, geometricLocation, interpolatedTangent,
        and interpolatedTangent2. 

        \beta Current Implementation assumes a flat bump map, setting
        the shadingNormal to the interpolatedNormal and the
        shadingLocation to the geometricLocation.
    */
    void sampleBump(const BumpMap::Ref& bump);


    /** Samples all fields. */
    void sample
    (const Material::Ref& material,
     const Point3&   geometricLocation,
     const Point3&   geometricNormal,
     const Vector3&  interpolatedNormal,
     const Vector2&  texCoord,
     const Vector3&  interpolatedTangent,
     const Vector3&  interpolatedTangent2);

    /** Samples all fields from a ray-triangle intersection. */
    void sample(const Tri::Intersector& intersector);
};

} // G3D

#endif
