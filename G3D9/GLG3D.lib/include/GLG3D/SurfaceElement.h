/**
  \file SurfaceElement.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-01-01
  \edited  2010-12-20

  Copyright 2000-2012, Morgan McGuire.
  All rights reserved.
 */
#ifndef GLG3D_SurfaceElement_h
#define GLG3D_SurfaceElement_h

#include "GLG3D/Tri.h"
#include "GLG3D/Material.h"

namespace G3D {

/** A sample of a surface at a point, describing the material
    and geometric properties needed for shading.
    
    This class abstracts the inputs to shading and scattering computations
    to simplify the implementation of a software renderer such as a 
    rasterizer, ray tracer, photon mapper, MLT, or path tracer.

    You can either create a SurfaceElement from a Tri::Intersector or
    create an uninitialized one and fill out the fields yourself.

    \sa SuperBSDF, SuperSurface, Material
    */
class SurfaceElement {
public:

    /** Infinite peak in the BSDF.  For use with getBSDFImpulses.*/
    class Impulse {
    public:
        /** Unit direction vector.  This points away from the
            intersection. */
        Vector3   w;

        /** \f$ f(\hat{\omega}_\mathrm{i}, \hat{\omega}_\mathrm{o})
            \mbox{max}(\hat{\omega}_\mathrm{i} \cdot \hat{n}, 0) /
            \delta(\hat{\omega}_\mathrm{o}, \hat{\omega}_\mathrm{o})
            \f$ for the impulse; the integral of the BSDF over a small
            area.  This is the factor to multiply scattered
            illumination by.

            For backwards recursive ray tracing, this is the
            weight of the recursive path's radiance. Do not
            multiply this by a cosine factor; that has already been
            factored in if necessary.*/
        Color3    magnitude;

        /** For use under refraction */
        float     eta;

        /** For use under refraction */
        Color3    extinction;
    };
    
    /** Post-bump map shading information. This is probably what you
        want to use if you're writing the shading code for a ray
        tracer or software renderer.*/
    struct Shading {
        Vector3 normal;
        Point2  texCoord;
        Point3  location;
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
        Point2  texCoord;
    } interpolated;

    /** Information about the true surface geometry. */
    struct Geometric {
        /** For a triangle, this is the face normal. This is useful
         for ray bumping */
        Vector3 normal;

        /** Actual location on the surface (it may be changed by
            displacement or bump mapping later. */
        Vector3 location;
    } geometric;

    /** Mostly for debugging */
    struct SourcePrimitive {
        int        index;
        float    u;
        float    v;
        SourcePrimitive() : index(-1), u(0), v(0) {}
    } sourcePrimitive;

    /** Screen space derivative of the texture coordinate.  \beta Currently unused. */
    Vector2    dTexCoorddX;

    /** Screen space derivative of the texture coordinate.  \beta Currently unused. */
    Vector2    dTexCoorddY;

    /** Sampled from the BSDF and emissive properties of a Material */
    class MaterialElement {
    public:
        /** May be NULL */
        Material::Ref  source;

        Color3     lambertianReflect;

        Color3     glossyReflect;

        /** Sampled from BSDF.  This is the glossy exponent on the range [0, inf] */
        float      glossyExponent;

        Color3     transmit;

        Color3     extinctionReflect;

        Color3     extinctionTransmit;

        float      etaTransmit;

        float      etaReflect;

        /** Partial coverage on the range [0, 1]; "alpha" value */
        float      coverage;
    
        /** Sampled from the emission map. */
        Radiance3  emit;
        
        /** Scales all elements by \a f.
           Useful for blending multiple surface elements together, e.g.:

           \code
              surfel.material = surfel1.material * 0.3f + surfel2.material * 0.6f + surfel3.material * 0.1f;
           \endcode
        */
        MaterialElement operator*(float f) const;

        /** Adds all elements. 
           Useful for blending multiple surface elements together.
           The <code>source</code> field is NULL.
           */
        MaterialElement operator+(const MaterialElement& m) const;

        MaterialElement() : glossyExponent(0.0f), etaTransmit(1.0f), etaReflect(1.0f), coverage(0.0f) {}

        /**
            \param lowFreq If true, sample from the average texture color
            instead of at each texel.  This can improve performance by
            increasing memory coherence. */
        MaterialElement(const SuperBSDF::Ref& bsdf, const Component3& emitMap, const Point2& texCoord, bool lowFreq = false);

    } material;
    
private:

    /**
       \param g Glossy exponent
       \param n Surface normal (world space)

       \return the intensity scale (nominally 1, but may be adjusted to take into account non-ideal importance sampling)
    */
    float glossyScatter
    (const Vector3& w_i,
     float          g,
     G3D::Random&   r,
     Vector3&       w_o) const;

    /** Sets the material.emit field by sampling at the shading.texCoord, leaving
        other fields of material unchanged. Called from set(Tri::Intersector, ...).*/
    void setEmit();
    
    /** Sets the SurfaceElement::shading fields, using the existing SurfaceElement::interpolated fields.

        Called from Called from set(Tri::Intersector, ...).

        \beta Current Implementation assumes a flat bump map, setting
        the shadingNormal to the interpolatedNormal and the
        shadingLocation to the geometricLocation.
    */
    void setBump(const BumpMap::Ref& bump, const Vector3& eye);

public:

    /** Computes F_r, given the cosine of the angle of incidence and 
       the reflectance at normal incidence. */
    static inline Color3 computeF(const Color3& F0, float cos_i) {
        return SuperBSDF::computeF(F0, cos_i);
    }

    SurfaceElement() {}

    SurfaceElement(const Tri::Intersector& intersector);
    
    /** Sets all fields. Called from the SurfaceElement(Tri::Intersector) constructor.*/
    void set
    (const Material::Ref& material,
     const Point3&   geometricLocation,
     const Point3&   geometricNormal,
     const Vector3&  interpolatedNormal,
     const Vector2&  texCoord,
     const Vector3&  interpolatedTangent,
     const Vector3&  interpolatedTangent2,
     const Vector3&  eye,
     int             index = -1,
     float             u = 0,
     float             v = 0);
    
     /** \brief Evaluate the finite portion of the BSDF: \f$(f_L + f_g)\f$. 

        Used for direct illumination.  Ignores impulses (delta
        functions) because for a random pair of directions, there is
        zero probability of sampling the delta function at a non-zero
        location, and an the infinite result would not be useful anyway.

        \param w_i \f$\hat{\omega}_i = \hat{\omega}_\mathrm{light}\f$ unit vector pointing to where
        the photon came from (often a the light source)

        \param w_o \f$\hat{\omega}_{o} = \hat{\omega}_{\mathrm{eye}}\f$ unit vector pointing forward
        towards where the photon is going (typically, the viewer)

        \param maxShininess Clamp specular exponent to this value.  For direct illumination, 1024 is recommended
        so that point lights create a visible highlight on mirrored surfaces.  For indirect illumination
        (e.g., in photon mapping), G3D::finf() is recommended so that sparse illumination samples
        do not result in bright haloed speckles on mirrors.

        \return Resulting radiance, with the alpha channel copied from
        the coverage mask.  Note that this does NOT factor the
        geometric \f$\hat{\omega}_\mathrm{i} \cdot \hat{n}\f$ term
        into the result.  Unmultipled alpha.
    */
    Color3 evaluateBSDF
    (const Vector3& w_i,
     const Vector3& w_o,
     const float    maxShininess = 1024.0f) const;

    /** 
        \brief Get the infinite peaks of the BSDF (usually refraction
        and mirror reflection).

        Used for Whitted backwards ray tracing with a small number of
        samples, where w_o = w_eye (pointing away from the
        intersection).  Distribution (stochastic) ray tracers should
        use the scatter() method instead.

        \param impulseArray Impulses are appended to this (it is <i>not</u>
        cleared first)
     */
    void getBSDFImpulses
    (const Vector3&  w_i,
     SmallArray<Impulse, 3>& impulseArray) const;

    /** \copydoc getBSDFImpulses */
    void getBSDFImpulses
    (const Vector3&  w_i,
     Array<Impulse>& impulseArray) const;

    /**
       \brief Sample outgoing photon direction \f$\vec{\omega}_o\f$ from the 
       distribution \f$f(\vec{\omega}_i, \vec{\omega}_o)\cos \theta_i\f$.

       Used in forward photon tracing and in path tracing.  The extra
       cosine term handles the projected area effect, which is almost always
       what you want to sample against.
       
       The probability of different kinds of scattering are given by:

       \f{eqnarray}
       \nonumber\rho_L &=& \int_\cap f_L (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i = 
\int_\cap \frac{1}{\pi} \rho_{L0} (1 - F_{r}(\vec{\omega}_i)) (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i \\
       \nonumber\rho_g &=& \int_\cap f_g (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i = 
\int_\cap \frac{s + 8}{8 \pi} F_r(\vec{\omega}_i)\max(0, \vec{n} \cdot \vec{\omega}_h)^{s} (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i \mbox{ if } s\neq\infty, \mbox{ otherwise } 0\\
       \nonumber\rho_m &=& \int_\cap f_m (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i = 
\int_\cap F_r(\vec{\omega}_i) \delta(\vec{\omega}_o, \vec{\omega}_m) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) ~d\vec{\omega}_i  \mbox{ if } s=\infty, \mbox{ otherwise } 0\\
       \nonumber\rho_t &=& \int_\cup f_t (\vec{\omega}_i \cdot \vec{n}) ~ d\vec{\omega}_i = 
\int_\cup (1 - F_r(\vec{\omega}_i)) T_0 \delta(\vec{\omega}_o, \vec{\omega}_t) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) ~d\vec{\omega}_i 
       \f}

       Note that at most one of the glossy and mirror probabilities may be non-zero.

       Not threadsafe unless \link Material::setStorage() setStorage\endlink(<code>COPY_TO_CPU</code>) has been called first.

       \param lowFreq If true, sample from the average texture color instead of at each texel.  This can
       improve performance by increasing memory coherence.

       \param eta_other Index of refraction on the side of the normal (i.e., material that is being exited to enter the 
         object whose surface this BSDF describes)

       \param weight_o Because samples are taken against the BSDF averaged over all wavelengths,
       the individual wavelengths always have distorted probabilities for a non-white surface.
       Moreover, for efficiency the scatter function does not perfectly importance sample.
       As a result, the distribution is distored and weight_o accounts for this.       

       For an incident photon with power_i, power_i * weight_o is the outgoing power adjusted to take
       this distortion into account. Note that when tracing radiance, you must also
       scale radiance by (eta_i / eta_o)^2.  That factor is not expressed in the weight.

       \param densityHack Returns a
       number that increases with the probability density of the
       selected sample but is not proportional to it.  It can't
       actually proportional because if a specular scattering event occured
       the density was infinite.  This is useful for algorithms like Image Space Photon Mapping
       that use coarse probability density as a heuristic for driving sampling patterns.
       See also conditionalScatteringProbability().

       @return false if the photon was absorbed, true if it scatters. */
    bool scatter
    (const Vector3& w_i,
     Vector3&       w_o,
     Color3&        weight_o,
     float&         eta_o,
     Color3&        extinction_o,
     Random&        random,
     float&         densityHack) const;


#if 0
    /**
       Returns the probability that an incoming photon will scatter,
       given its direction.

       Evaluates the integral
       \f$ \int_\cap f(\vec{\omega}_i, \vec{\omega}_o) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i \f$
       whose value is on the interval \f$[0, 2\pi]\f$.

       Combined with evaluateBSDF and getBSDFImpulses, this allows exact
       computation of the probability density for a scattered photon, which
       is needed for algorithms like path tracing and metropolis light transport.
     */
    Color3 conditionalScatteringProbability(const Vector3& w_i) const;
#endif
};

} // G3D

#endif
