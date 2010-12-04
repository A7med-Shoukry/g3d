#include "Hit.h"

Hit::Hit() : position(Vector3::inf()), material(NULL) {
}


void Hit::setFromIntersector(const Tri::Intersector& intersector) {
    if (intersector.tri != NULL) {
        
        SurfaceSample sample(intersector);

        position = sample.shading.location;
        normal = sample.shading.normal;
        texCoord = sample.shading.texCoord;
        material = sample.material;
    } else {
        position = Vector3::inf();
    }
}
