/**
  \file SurfaceSample.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-01-01
  \edited  2010-12-20

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#ifndef GLG3D_SurfaceSample_h
#define GLG3D_SurfaceSample_h

#include "GLG3D/Tri.h"
#include "GLG3D/Material.h"

namespace G3D {

/** A sample of a surface at a point, describing the material
    and geometric properties needed for shading.
    
    This class abstracts the inputs to shading and scattering computations
    to simplify the implementation of a software renderer such as a 
    rasterizer, ray tracer, photon mapper, MLT, or path tracer.

    You can either create a SurfaceSample from a Tri::Intersector or
    create an uninitialized one and fill out the fields yourself if not
    using SuperBSDF directly.

    \sa SuperBSDF, SuperSurface
    */
class SurfaceSample {
public:
    /** May be NULL */
    Material::Ref material;
#if 0
    /** Location after bump map is applied. \deprecated */
    Point3     shadingLocation;
    
    /** Normal after normal map is applied. \deprecated*/
    Vector3    shadingNormal;

    /** Smoothed (e.g., interpolated vertex normals) normal for curved surfaces. \deprecated*/
    Vector3    interpolatedNormal;

    /** Location before bump map is applied. \deprecated*/
    Vector3    geometricLocation;

    /** Normal to the true surface (e.g., containing triangle face)
        at the shading location. \deprecated*/
    Vector3    geometricNormal;
    
    /** Approximately orthogonal to interpolatedNormal. \deprecated*/
    Vector3    interpolatedTangent;
    
    /** Approximately orthogonal to interpolatedNormal and interpolatedTangent. \deprecated*/
    Vector3    interpolatedTangent2;

    /** Texture coordinate \deprecated*/
    Vector2    texCoord;
#endif

    /** Post-bump map shading information. This is probably what you
        want to use if you're writing the shading code for a ray
        tracer or software renderer.*/
    struct Shading {
        Vector3 normal;
        Vector2 texCoord;
        Vector3 location;
    } shading;

    /** Pre-bump map, interpolated attributes. These are probably not
     what you want to use for shading--see the \a shading field
     instead.*/
    struct Interpolated {
        /** The interpolated vertex normal. */
        Vector3 normal;
        Vector3 tangent;

        /** This is the second tangent for parallax mapping (the
            "bitangent".) */
        Vector3 tangent2;
        Vector2 texCoord;
    } interpolated;

    struct Geometric {
        /** For a triangle, this is the face normal. This is useful
         for ray bumping */
        Vector3 normal;

        /** Actual location on the surface (it may be changed by
            displacement or bump mapping later. */
        Vector3 location;
    } geometric;

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

    /** Sampled from BSDF.  This is the packed value on the range [0, 1] */
    float      glossyExponent;

    /** Sampled from BSDF */
    Color3     transmit;

    /** Sampled from BSDF */
    Color3     extinctionReflect;

    /** Sampled from BSDF */
    Color3     extinctionTransmit;

    /** Sampled from BSDF */
    float      etaTransmit;

    /** Sampled from BSDF */
    float      etaReflect;

    /** Sampled from the emission map. */
    Radiance3  emit;

    SurfaceSample() : coverage(0.0f), glossyExponent(0.0f), etaTransmit(1.0f), etaReflect(1.0f) {}

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
    void sampleBump(const BumpMap::Ref& bump, const Vector3& eye);

    /** Samples all fields. */
    void sample
    (const Material::Ref& material,
     const Point3&   geometricLocation,
     const Point3&   geometricNormal,
     const Vector3&  interpolatedNormal,
     const Vector2&  texCoord,
     const Vector3&  interpolatedTangent,
     const Vector3&  interpolatedTangent2,
     const Vector3&  eye);

    /** Samples all fields from a ray-triangle intersection. */
    void sample(const Tri::Intersector& intersector);
};

} // G3D

#endif
