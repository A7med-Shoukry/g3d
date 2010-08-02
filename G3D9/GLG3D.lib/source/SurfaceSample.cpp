#include "GLG3D/SurfaceSample.h"

namespace G3D {


SurfaceSample::SurfaceSample(const Tri::Intersector& intersector) {
    sample(intersector);
}
    

void SurfaceSample::sampleEmit(const Component3& emitMap) {
    emit = emitMap.sample(texCoord);
}


void SurfaceSample::sampleBSDF(const SuperBSDF::Ref& bsdf) {
    const Color4& packD = bsdf->lambertian().sample(texCoord);
    coverage           = packD.a;
    lambertianReflect  = packD.rgb();
    
    const Color4& packG = bsdf->specular().sample(texCoord);
    glossyReflect      = packG.rgb();
    glossyExponent     = SuperBSDF::unpackSpecularExponent(packG.a);
    
    transmit = bsdf->transmissive().sample(texCoord);
    
    extinctionReflect = bsdf->extinctionReflect();
    extinctionTransmit = bsdf->extinctionTransmit();
}
    

void SurfaceSample::sampleBump(const BumpMap::Ref& bump) {
    // TODO: Bump mapping goes here
    shadingNormal = interpolatedNormal;
    shadingLocation = geometricLocation;
}


void SurfaceSample::sample
(const Material::Ref& material,
 const Point3&   geometricLocation,
 const Point3&   geometricNormal,
 const Vector3&  interpolatedNormal,
 const Vector2&  texCoord,
 const Vector3&  interpolatedTangent,
 const Vector3&  interpolatedTangent2) {
    this->texCoord = texCoord;
    this->geometricLocation = geometricLocation;
    this->interpolatedNormal = interpolatedNormal;
    this->geometricNormal = geometricNormal;
    this->interpolatedTangent = interpolatedTangent;
    this->interpolatedTangent2 = interpolatedTangent2;
    
    sampleEmit(material->emissive());
    sampleBSDF(material->bsdf());
    sampleBump(material->bump());
}
    
    
void SurfaceSample::sample(const Tri::Intersector& intersector) {
    debugAssert(intersector.tri != NULL);

    Point3 P;
    Vector3 n;
    Vector2 texCoord;
    Vector3 t1, t2;
    intersector.getResult(P, n, texCoord, t1, t2);
    sample(intersector.tri->material(), P, intersector.tri->normal(), n, texCoord, t1, t2);
}

} // G3D
