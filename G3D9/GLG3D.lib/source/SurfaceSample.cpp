/**
  \file SurfaceElement.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-01-01
  \edited  2010-12-20

  Copyright 2000-2011, Morgan McGuire.
  All rights reserved.
 */
#include "GLG3D/SurfaceSample.h"

namespace G3D {

#define INV_PI  (0.318309886f)
#define INV_8PI (0.0397887358f)

SurfaceElement::SurfaceElement(const Tri::Intersector& intersector) {
    debugAssert(intersector.tri != NULL);

    Point3 P;
    Vector3 n;
    Vector2 texCoord;
    Vector3 t1, t2;
    intersector.getResult(P, n, interpolated.texCoord, t1, t2);
    set(intersector.tri->material(), P, intersector.tri->normal(), n, interpolated.texCoord, t1, t2, intersector.eye);
}

    
SurfaceElement::MaterialElement::MaterialElement(const SuperBSDF::Ref& bsdf, const Component3& emitMap, const Point2& texCoord, bool loqFreq) {
    const Color4& packD = bsdf->lambertian().sample(texCoord);
    coverage           = packD.a;
    lambertianReflect  = packD.rgb();

    // TODO: implement low frequency option

    const Color4& packG = bsdf->specular().sample(texCoord);
    glossyReflect      = packG.rgb();
    glossyExponent     = SuperBSDF::unpackSpecularExponent(packG.a);
    
    transmit = bsdf->transmissive().sample(texCoord);

    etaTransmit = bsdf->etaTransmit();
    etaReflect = bsdf->etaReflect();
    extinctionReflect = bsdf->extinctionReflect();
    extinctionTransmit = bsdf->extinctionTransmit();

    emit = emitMap.sample(texCoord);
}
    

SurfaceElement::MaterialElement SurfaceElement::MaterialElement::operator*(float f) const {
    MaterialElement e;

    e.coverage           = coverage * f;
    e.emit               = emit * f;
    e.etaReflect         = etaReflect * f;
    e.etaTransmit        = etaTransmit * f;
    e.extinctionReflect  = extinctionReflect * f;
    e.extinctionTransmit = extinctionTransmit * f;
    e.glossyExponent     = glossyExponent * f;
    e.glossyReflect      = glossyReflect * f;
    e.lambertianReflect  = lambertianReflect * f;

    return e;
}


SurfaceElement::MaterialElement SurfaceElement::MaterialElement::operator+(const MaterialElement& m) const {
    MaterialElement e;

    e.coverage           = coverage           + m.coverage;
    e.emit               = emit               + m.emit;
    e.etaReflect         = etaReflect         + m.etaReflect;
    e.etaTransmit        = etaTransmit        + m.etaTransmit;
    e.extinctionReflect  = extinctionReflect  + m.extinctionReflect;
    e.extinctionTransmit = extinctionTransmit + m.extinctionTransmit;
    e.glossyExponent     = glossyExponent     + m.glossyExponent;
    e.glossyReflect      = glossyReflect      + m.glossyReflect;
    e.lambertianReflect  = lambertianReflect  + m.lambertianReflect;

    return e;
}

void SurfaceElement::setBump(const BumpMap::Ref& bump, const Vector3& eye) {
#if 0
    // Disabled until Morgan adjusts SuperBSDF to handle surface normals pointing away from eye
    if (bump.notNull()) {
        const Image4::Ref& normalMap = bump->normalBumpMap()->image();

        // Build world -> tangent space transform
        const Matrix3 toTangentSpace(interpolatedTangent.x,  interpolatedTangent.y,  interpolatedTangent.z,
                                     interpolatedTangent2.x, interpolatedTangent2.y, interpolatedTangent2.z,
                                     interpolatedNormal.x,   interpolatedNormal.y,   interpolatedNormal.z);

        if (bump->settings().iterations == 0) {
            // Normal Mapping
            shading.texCoord = texCoord;

        } else if (bump->settings().iterations > 0) {
            // Parallax Mapping

            // Get bump height from alpha channel in [0 to 1] range

            float sampledBumpHeight = normalMap->bilinear(texCoord * Vector2(normalMap->width(), normalMap->height())).a;

            // Adjust height range to [-0.5 to 0.5]
            // Then adjust the range based off the bias and scale
            sampledBumpHeight = (sampledBumpHeight - 0.5f + bump->settings().bias) * bump->settings().scale;

            // We need the direction pointing from sample to eye in tangent space
            const Vector3& tanEye = toTangentSpace * (-eye);

            // Adjust texture coordinate based on the distance along the local XY plane (Y-axis is inverted in tangent space)
            // This helps the texture coordinates reflect where the viewer is actually looking into the heightfield
            shading.texCoord = texCoord + (sampledBumpHeight * Vector2(tanEye.x, -tanEye.y));

        } else {
            // Relief Mapping/Parallax Occusion Mapping
            // 
            // Not implemented, parallax mapping catches this case
        }

        // Sample normal map with (possibly adjusted) texture coordinate and map back to [-1 to 1] from [0 to 1] texture range
        Vector2int32 mappedTexCoords(shading.texCoord * Vector2(normalMap->width(), normalMap->height()));
        const Color3& sampledNormal = normalMap->get(mappedTexCoords.x, mappedTexCoords.y).rgb() * Color3(2.0f, 2.0f, 2.0f) + Color3(-1.0f, -1.0f, -1.0f);

        // Build tangent -> world space transform
        const Matrix3& toWorldSpace = toTangentSpace.transpose();

        // Transform normal from tangent space to world space
        shading.normal = shadingNormal = (toWorldSpace * Vector3(sampledNormal.r, sampledNormal.g, sampledNormal.b)).direction();
    } else {
        // Not adjusted if no bump map is defined
        shading.normal = shadingNormal = interpolatedNormal;
        shading.texCoord = texCoord;
    }

    // Shading location is same as geometric location
    shading.location = shadingLocation = geometricLocation;
#else
    shading.normal   = interpolated.normal;
    shading.location = geometric.location;
    shading.texCoord = interpolated.texCoord;
#endif
}


void SurfaceElement::set
(const Material::Ref& material,
 const Point3&   geometricLocation,
 const Point3&   geometricNormal,
 const Vector3&  interpolatedNormal,
 const Vector2&  texCoord,
 const Vector3&  interpolatedTangent,
 const Vector3&  interpolatedTangent2,
 const Vector3&  eye) {
    this->material.source = material;
    interpolated.texCoord = texCoord;
    interpolated.normal   = interpolatedNormal;
    geometric.location    = geometricLocation;
    geometric.normal      = geometricNormal;
    
    setBump(material->bump(), eye);
    this->material = MaterialElement(material->bsdf(), material->emissive(), shading.texCoord);
}
    
   
Color3 SurfaceElement::evaluateBSDF
(const Vector3& w_i,
 const Vector3& w_o,
 const float    maxShininess) const {
    
    const Vector3& n = shading.normal;
    const float cos_i = abs(w_i.dot(n));
    
    Color3 S(Color3::zero());
    Color3 F(Color3::zero());
    if ((material.glossyExponent != 0) && (material.glossyReflect.nonZero())) {
        // Glossy

        // Half-vector
        const Vector3& w_h = (w_i + w_o).direction();
        const float cos_h = max(0.0f, w_h.dot(n));
        
        const float s = min(material.glossyExponent, material.glossyExponent);
        
        F = computeF(material.glossyReflect, cos_i);
        if (s == finf()) {
            S = Color3::zero();
        } else {
            S = F * (powf(cos_h, s) * (s + 8.0f) * INV_8PI);
        }
    }

    const Color3& D = (material.lambertianReflect * INV_PI) * (Color3(1.0f) - F);

    Color3 f;
    if ((w_i.dot(n) >= 0) == (w_o.dot(n) >= 0)) {
        // Rays are on the same side of the normal
        f += S + D;
    }

    return f;
}


void SurfaceElement::getBSDFImpulses
(const Vector3&  w_i,
 Array<Impulse>& impulseArray) const {
    SmallArray<Impulse, 3> temp;
    getBSDFImpulses(w_i, temp);
    impulseArray.resize(temp.size());
    for (int i = 0; i < impulseArray.size(); ++i) {
        impulseArray[i] = temp[i];
    }
}


void SurfaceElement::getBSDFImpulses
(const Vector3&  w_i,
 SmallArray<Impulse, 3>& impulseArray) const {

    const Vector3& n = shading.normal;
    debugAssert(w_i.isUnit());
    debugAssert(n.isUnit());

    Color3 F(0,0,0);
    bool Finit = false;

    ////////////////////////////////////////////////////////////////////////////////
    if (material.glossyReflect.nonZero()) {
        // Cosine of the angle of incidence, for computing F
        const float cos_i = max(0.001f, w_i.dot(n));
        F = computeF(material.glossyReflect, cos_i);
        Finit = true;
            
        if (material.glossyExponent == inf()) {
            // Mirror                
            Impulse& imp     = impulseArray.next();
            imp.w            = w_i.reflectAbout(n);
            imp.coefficient  = F;
            imp.eta          = material.etaReflect;
            imp.extinction   = material.extinctionReflect;
            debugAssert(imp.w.isUnit());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    if (material.transmit.nonZero()) {
        // Fresnel transmissive coefficient
        Color3 F_t;

        if (Finit) {
            F_t = (Color3::one() - F);
        } else {
            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.001f, w_i.dot(n));
            // F = lerp(0, 1, pow5(1.0f - cos_i)) = pow5(1.0f - cos_i)
            // F_t = 1 - F
            F_t.r = F_t.g = F_t.b = 1.0f - pow5(1.0f - cos_i);
        }

        // Sample transmissive
        const Color3& T0 = material.transmit;
        const Color3& p_transmit  = F_t * T0;
       
        // Disabled; interpolated normals can be arbitrarily far out
        //debugAssertM(w_i.dot(n) >= -0.001, format("w_i dot n = %f", w_i.dot(n)));
        Impulse& imp     = impulseArray.next();

        imp.coefficient  = p_transmit;
        imp.w            = (-w_i).refractionDirection(n, material.etaTransmit, material.etaReflect);
        imp.eta          = material.etaTransmit;
        imp.extinction   = material.extinctionTransmit;
        if (imp.w.isZero()) {
            // Total internal refraction
            impulseArray.popDiscard();
        } else {
            debugAssert(imp.w.isUnit());
        }
    }
}


float SurfaceElement::glossyScatter
(const Vector3& w_i,
 float          g,
 G3D::Random&   r,
 Vector3&       w_o) const {

    const Vector3& n = shading.normal;

    // Notation translator from the jgt paper:
    //
    //      n_u = n_v = g
    //      k1 = w_i
    //      k2 = w_o
    //      h  = w_h
    float intensity;
/*
    // TODO: remove (hack to be diffuse)
    w_o = Vector3::cosHemiRandom(n, r);
    return 1.0f;

    // Rejection sampling:
    do {
        w_o = Vector3::cosHemiRandom(n, r);
        Vector3 w_h = (w_i + w_o).direction();
        intensity = powf(w_h.dot(n), g);
    } while (r.uniform() > intensity);
    return 1.0f;
*/
    do {
        // Eq. 6 and 10 (eq. 9 cancels for isotropic)
        // Generate a cosine distributed half-vector:
        const Vector3& w_h = Vector3::cosPowHemiRandom(n, g, r);

        // Now transform to the output vector: (eq. 7)
        w_o = w_i.reflectAbout(w_h);

        // The output vector has three problems (with solutions used in
        // this implementation):
        //
        //   1. Distribution is off because measures in w_h and w_o space
        //        don't line up (Rejection sample by discrepancy)
        //
        //   2. May be below the surface of the plane (Loop until a sample
        //        is found above; the scatter function's choice of glossy
        //        scattering means that this method is conditioned on a
        //        bounce occuring).  Since when w_h = n, w_o = mirror
        //        reflection vector, there always exists some probability
        //        distribution above the plane.
        //
        //   3. Does not account for the n.dot(w_o) probability (Rejection
        //        sample by discrepancy)

        // Adjust for the measure difference between w_o and w_h spaces (eq. 8)
        intensity = 4.0f * w_i.dot(w_h);
        if (intensity <= 0.0f) {
            // Because shading normals are different from geometric normals, we might
            // end up with negative intensity.
            return 0.0f;
        }

    } while (r.uniform() > w_o.dot(n));

    return intensity;
}

#if 0
    // Let phi be the angle about the normal
    const float phi = r.uniform(0, G3D::twoPi());
    const float cos_phi = cos(phi);
    const float sin_phi = sin(phi);

    // Rejection sampling of angle relative to normal
    while (true) {
        const float cos_theta = pow(r.uniform(0.0f, 1.0f), 1.0f / (g + 1.0f));
        const float sin_theta = sqrtf(1.0f - square(cos_theta));
        
        // In the reference frame of the surface
        const Vector3 h_tangentSpace(cos_phi * sin_theta, sin_phi * sin_theta, cos_theta);
        
        const Vector3& h = h_tangentSpace;//TODO;
        
        // Set the attenuation to ps from the paper, computed based on
        // the monte carlo section of the paper
        const Vector3& k1 = w_i;
        const float hdotk = h.dot(k1);
        
        if (hdotk > 0.0f) {
            // On the front side of the specular lobe; we can continue        
            Vector3 k2 = (-w_i + 2.0f * hdotk * h).direction();
        
            // Ensure that we're above the plane of the surface
            if (k2.dot(n) > 0.0f) {

                // Compute the density of the perturbed ray
                const float hdotn = n.dot(h);
                const float factor1 = (g + 1.0f) / G3D::twoPi();
                const float factor2 = pow(hdotn, g);
                
                const float inv_actual_density = (4.0 * hdotk) / (factor1 * factor2);
                
                // Compute the density of what we actually want (from the BRDF)
                float brdf;
                //AshikminShirleyAnisotropicPhongBRDF::ComputeDiffuseSpecularFactors( diffuseFactor, brdf, k2, ri, NU, NV, Rs );
                
                // Now we need to correct for the fact that we sampled against
                // the wrong distribution by a factor of N dot L.
                float specFactor = inv_actual_density * brdf;
                
                //specular.ray.Set( ri.ptIntersection, k2 );
            }
        }
    }

    return true;
}
#endif

bool SurfaceElement::scatter
(const Vector3& w_i,
 const Color3&  power_i,
 Vector3&       w_o,
 Color3&        power_o,
 float&         eta_o,
 Color3&        extinction_o,
 Random&        random,
 float&         density) const {

    const Vector3& n = shading.normal;

    if (false) {  
        // TODO: Remove
        // Testing code to generate Russian roulette scattering
        w_o = Vector3::cosHemiRandom(n, random);
        power_o = evaluateBSDF(w_i, w_o).rgb() * power_i;
        if (power_o.average() > random.uniform()) {
            power_o /= power_o.average();
            debugAssert(power_o.r >= 0.0f);
            return true;
        } else {
            return false;
        }
     }

    // TODO: glossy bounce should be taken before lambertian

    // Choose a random number on [0, 1], then reduce it by each kind of
    // scattering's probability until it becomes negative (i.e., scatters).
    float r = random.uniform();

    ////////////////////////////////////////////////////////////////////////////////
    if (material.lambertianReflect.nonZero()) {
        
        alwaysAssertM(material.coverage > 0.0f, "Scattered from an alpha masked location");
        float p_LambertianAvg = material.lambertianReflect.average();
        
        r -= p_LambertianAvg;
        
        if (r < 0.0f) {
            // Lambertian scatter
            
            // (Cannot hit division by zero because the if prevents this
            // case when p_LambertianAvg = 0)
            power_o         = power_i * material.lambertianReflect / p_LambertianAvg;
            w_o             = Vector3::cosHemiRandom(n, random);
            density         = p_LambertianAvg * 0.01f;
            eta_o           = material.etaReflect;
            extinction_o    = material.extinctionReflect;
            debugAssert(power_o.r >= 0.0f);

            return true;
        }
    }

    Color3 F(0, 0, 0);
    bool Finit = false;

    ////////////////////////////////////////////////////////////////////////////////
    if (material.glossyReflect.nonZero()) {
            
        // Cosine of the angle of incidence, for computing F
        const float cos_i = max(0.001f, w_i.dot(n));
        F = computeF(material.glossyReflect, cos_i);
        Finit = true;

        const Color3& p_specular = F;
        const float p_specularAvg = p_specular.average();

        r -= p_specularAvg;
        if (r < 0.0f) {
            if (material.glossyExponent != finf()) {
                // Glossy
                float intensity = (glossyScatter(w_i, material.glossyExponent, random, w_o) / p_specularAvg);
                if (intensity <= 0.0f) {
                    // Absorb
                    return false;
                }
                power_o = p_specular * power_i * intensity;
                density = p_specularAvg * 0.1f;

            } else {
                // Mirror

                w_o = w_i.reflectAbout(n);
                power_o = p_specular * power_i * (1.0f / p_specularAvg);
                density = p_specularAvg;
            }
            debugAssert(power_o.r >= 0.0f);

            eta_o = material.etaReflect;
            extinction_o = material.extinctionReflect;
            return true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    if (material.transmit.nonZero()) {
        // Fresnel transmissive coefficient
        Color3 F_t;

        if (Finit) {
            F_t = (Color3::one() - F);
        } else {
            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.001f, w_i.dot(n));
            // F = lerp(0, 1, pow5(1.0f - cos_i)) = pow5(1.0f - cos_i)
            // F_t = 1 - F
            F_t.r = F_t.g = F_t.b = 1.0f - pow5(1.0f - cos_i);
        }

        const Color3& T0          = material.transmit;
        
        const Color3& p_transmit  = F_t * T0;
        const float p_transmitAvg = p_transmit.average();
        
        r -= p_transmitAvg;
        if (r < 0.0f) {
            power_o      = p_transmit * power_i * (1.0f / p_transmitAvg);
            w_o          = (-w_i).refractionDirection(n, material.etaTransmit, material.etaReflect);
            density      = p_transmitAvg;
            eta_o        = material.etaTransmit;
            extinction_o = material.extinctionTransmit;

            debugAssert(w_o.isZero() || ((w_o.dot(n) < 0) && w_o.isUnit()));
            debugAssert(power_o.r >= 0.0f);

            // w_o is zero on total internal refraction
            return ! w_o.isZero();
        }
    }

    // Absorbed
    return false;
}


Color3 SurfaceElement::conditionalScatteringProbability(const Vector3& w_i) const {
    const Vector3& n = shading.normal;
    Color3 F(0, 0, 0);
    bool Finit = false;

    Color3 total;
    const float cos_i = max(0.0f, w_i.dot(n));

    if (material.glossyReflect.nonZero()) {
        // Glossy reflectance
        F = computeF(material.glossyReflect, max(0.001f, cos_i));
        Finit = true;

        if (isFinite(material.glossyExponent)) {
            // The cosine appears in the glossy lobe
            total += F * cos_i;
        } else {
            // The mirror BSDF term is defined to divide out the cosine
            // of the incident angle.
            total += F;
        }
    }

    // Lambertian
    total += (Color3(1.0f) - F) * material.lambertianReflect * cos_i;

    // Transmission; defined to divide out the cosine term as well
    total += (Color3(1.0f) - F) * material.transmit;

    // The total is at most 1/2pi, so that if integrated over
    // the entire incoming hemisphere the max is 1.0.
    return total / (2.0f * pif());
}

} // G3D
