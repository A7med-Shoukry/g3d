/**
 \file      SuperBSDF.cpp
 \author    Morgan McGuire, http://graphics.cs.williams.edu

 \created   2008-08-10
 \edited    2011-6-28
*/
#include "GLG3D/SuperBSDF.h"

namespace G3D {

float SuperBSDF::ignoreFloat;

#define INV_PI  (0.318309886f)
#define INV_8PI (0.0397887358f)


SuperBSDF::Ref SuperBSDF::speedCreate(BinaryInput& b) {
    SuperBSDF::Ref s = new SuperBSDF();
    
    std::string header = b.readString32();
    alwaysAssertM(header == "SuperBSDF", "SuperBSDF::speedCreate called with something that was not a SuperBSDF");
    s->m_lambertian.speedDeserialize(b);
    s->m_specular.speedDeserialize(b);
    s->m_transmissive.speedDeserialize(b);
    s->m_eta_t = b.readFloat32();
    s->m_extinction_t.deserialize(b);
    s->m_eta_r = b.readFloat32();
    s->m_extinction_r.deserialize(b);

    return s;
}


void SuperBSDF::speedSerialize(BinaryOutput& b) const {
    b.writeString32("SuperBSDF");

    m_lambertian.speedSerialize(b);
    m_specular.speedSerialize(b);
    m_transmissive.speedSerialize(b);
    b.writeFloat32(m_eta_t);
    m_extinction_t.serialize(b);
    b.writeFloat32(m_eta_r);
    m_extinction_r.serialize(b);
}


SuperBSDF::Ref SuperBSDF::create
(const Component4& lambertian,
 const Component4& specular,
 const Component3& transmissive,
 float             eta_t,
 const Color3&     extinction_t,
 float             eta_r,
 const Color3&     extinction_r) {

    SuperBSDF::Ref bsdf      = new SuperBSDF();

    bsdf->m_lambertian      = lambertian;
    bsdf->m_specular        = specular;
    bsdf->m_transmissive    = transmissive;
    bsdf->m_eta_t           = eta_t;
    bsdf->m_extinction_t    = extinction_t;
    bsdf->m_eta_r           = eta_r;
    bsdf->m_extinction_r    = extinction_r;

    return bsdf;
}


void SuperBSDF::setStorage(ImageStorage s) const {
    m_lambertian.setStorage(s);
    m_transmissive.setStorage(s);
    m_specular.setStorage(s);
}


bool SuperBSDF::similarTo(const SuperBSDF::Ref& other) const {
    return 
        (m_lambertian.factors()   == other->m_lambertian.factors()) &&
        (m_transmissive.factors() == other->m_transmissive.factors()) &&
        (m_specular.factors()     == other->m_specular.factors());
}


bool SuperBSDF::hasMirror() const {
    const Color4& m = m_specular.max();
    return (m.a == 1.0f) && ! m.rgb().isZero();
}


bool SuperBSDF::hasGlossy() const {
    float avg = m_specular.mean().a;
    return (avg > 0) && (avg < 1) && ! m_specular.max().rgb().isZero();
}


bool SuperBSDF::hasLambertian() const {
    return ! m_lambertian.max().rgb().isZero();
}

}
