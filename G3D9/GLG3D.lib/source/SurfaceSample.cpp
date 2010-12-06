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

    etaTransmit = bsdf->etaTransmit();
    etaReflect = bsdf->etaReflect();
    extinctionReflect = bsdf->extinctionReflect();
    extinctionTransmit = bsdf->extinctionTransmit();
}
    

void SurfaceSample::sampleBump(const BumpMap::Ref& bump, const Vector3& eye) {
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
    shading.normal = shadingNormal = interpolatedNormal;
    shading.location = shadingLocation = geometricLocation;
    shading.texCoord = texCoord;
#endif
}


void SurfaceSample::sample
(const Material::Ref& material,
 const Point3&   geometricLocation,
 const Point3&   geometricNormal,
 const Vector3&  interpolatedNormal,
 const Vector2&  texCoord,
 const Vector3&  interpolatedTangent,
 const Vector3&  interpolatedTangent2,
 const Vector3&  eye) {
    this->material        = material;
    interpolated.texCoord = this->texCoord = texCoord;
    geometric.location    = this->geometricLocation = geometricLocation;
    geometric.normal      = this->geometricNormal = geometricNormal;
    interpolated.normal   = this->interpolatedNormal = interpolatedNormal;
    interpolated.tangent  = this->interpolatedTangent = interpolatedTangent;
    interpolated.tangent2 = this->interpolatedTangent2 = interpolatedTangent2;
    
    sampleEmit(material->emissive());
    sampleBSDF(material->bsdf());
    sampleBump(material->bump(), eye);
}
    
    
void SurfaceSample::sample(const Tri::Intersector& intersector) {
    debugAssert(intersector.tri != NULL);

    Point3 P;
    Vector3 n;
    Vector2 texCoord;
    Vector3 t1, t2;
    intersector.getResult(P, n, interpolated.texCoord, t1, t2);
    sample(intersector.tri->material(), P, intersector.tri->normal(), n, interpolated.texCoord, t1, t2, intersector.eye);
}

} // G3D
