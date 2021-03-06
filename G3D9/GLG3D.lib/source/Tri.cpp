/**
  \file GLG3D.lib/source/Tri.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-05-25
  \edited  2012-03-16
 */ 
#include "GLG3D/Tri.h"
#include "G3D/Ray.h"
#include "GLG3D/Surface.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/CPUVertexArray.h"

namespace G3D {
    



Tri::Tri(const int i0, const int i1, const int i2,
         const CPUVertexArray&       vertexArray,
         const Proxy<Material>::Ref& material,
         bool                        twoSided) :
    m_material(material) {
    debugAssert(material.isNull() ||
                (isValidHeapPointer(material.pointer()) &&
                 (material.pointer() > (void*)0xf)));
    index[0] = i0;
    index[1] = i1;
    index[2] = i2;

    m_area = e1(vertexArray).cross(e2(vertexArray)).length() * 0.5f;

    // Encode the twoSided flag into the sign bit of m_area
    m_area = m_area * (twoSided ? -1.0f : 1.0f);

}


Triangle Tri::toTriangle(const CPUVertexArray& vertexArray) const {
    return Triangle(position(vertexArray, 0), position(vertexArray, 1), position(vertexArray, 2));
}

#ifdef _MSC_VER
// Turn on fast floating-point optimizations
#pragma float_control( push )
#pragma fp_contract( on )
#pragma fenv_access( off )
#pragma float_control( except, off )
#pragma float_control( precise, off )
#endif

bool Tri::Intersector::operator()(const Ray& ray, const CPUVertexArray& vertexArray, const Tri& tri, 
                                  bool twoSided, float& distance) {
    // See RTR3 p.746 (RTR2 ch. 13.7) for the basic algorithm.
    static const float EPS = 1e-12f;

    // How much to grow the edges of triangles by to allow for small roundoff.
    static const float conservative = 1e-8f;

    // Get all vertex attributes from these to avoid unneccessary pointer indirection
    const CPUVertexArray::Vertex& vertex0 = tri.vertex(vertexArray, 0);
    const CPUVertexArray::Vertex& vertex1 = tri.vertex(vertexArray, 1);
    const CPUVertexArray::Vertex& vertex2 = tri.vertex(vertexArray, 2);
    
    const Vector3& v0 = vertex0.position;
    const Vector3& e1 = vertex1.position - v0;
    const Vector3& e2 = vertex2.position - v0;


    // Test for backfaces first because this eliminates 50% of all triangles.

    // This test is equivalent to n.dot(ray.direction()) >= -EPS
    // Where n is the face unit normal, which we do not explicitly store
    // The first two check whether we should treat the tri as double sided
    if (!twoSided && (tri.m_area >= 0) && (e1.cross(e2)).dot(ray.direction()) >= -EPS * 2.0f * tri.m_area) {
        // Backface or nearly parallel
        return false;
    }

    const Vector3& p = ray.direction().cross(e2);

    // Will be negative if we are coming from the back.
    const float a = e1.dot(p);

    // Divide by a
    const float f = 1.0f / a;
    const float c = conservative * f;

    const Vector3& s = (ray.origin() - v0)*f;
    const float u = s.dot(p);

    // Note: (ua > a) == (u > 1). Delaying the division by a until
    // after all u-v tests have passed gives a 6% speedup.
    if ((u < -c) || (u > 1 + c)) {
        // We hit the plane of the triangle, but outside the triangle
        return false;
    }

    const Vector3& q = s.cross(e1);
    const float v = ray.direction().dot(q);

    if ((v < -c) || ((u + v) > 1 + c)) {
        // We hit the plane of the triangle, but outside the triangle.
        return false;
    }

    if (abs(a) < EPS) {
        // This ray was parallel, but passed the backface test. We don't test until
        // down here because this case happens really infrequently.
        return false;
    }

    
    const float t = e2.dot(q);

    if ((t > 0.0f) && (t < distance)) {
        // Alpha masking
 
        if (alphaTest) {
            const Material::Ref& material = tri.material();

            if (material.notNull()) {
                const Component4& lambertian = material->bsdf()->lambertian();
                if (lambertian.min().a < alphaThreshold) {
                    // This texture has an alpha channel; we have to test against it.
                    const float w = 1.0f - u - v;
            

                    Point2 texCoord = 
                        w * vertex0.texCoord0 + 
                        u * vertex1.texCoord0 +
                        v * vertex2.texCoord0;
            
                    const Image4::Ref& image = lambertian.image();
                    texCoord.x *= image->width();
                    texCoord.y *= image->height();

                    if (image->nearest(texCoord).a < alphaThreshold) {
                        // Alpha masked location--passed through this tri
                        return false;
                    }

                    // Hit the opaque part
                }
            }
        }
        
        // This is a new hit.  Save away the data about the hit
        // location (including if we hit the backside), but don't bother computing barycentric w,
        // the hit location or the normal until after we've checked
        // against all triangles.

        distance = t;
        this->tri = &tri;

        eye = ray.direction();
        this->u = u;
        this->v = v;
        this->backside = (a < 0);
        return true;
    } else {
        return false;
    }
}


void Tri::Intersector::getResult
(const CPUVertexArray& vertexArray,
 Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord) const {

    debugAssert(tri);
    float w = 1.0 - u - v;


    const CPUVertexArray::Vertex& vert0 = tri->vertex(vertexArray, 0);
    const CPUVertexArray::Vertex& vert1 = tri->vertex(vertexArray, 1);
    const CPUVertexArray::Vertex& vert2 = tri->vertex(vertexArray, 2);

    const Point3& v0 = vert0.position;

    location = v0 + 
               u * (vert1.position - v0) +
               v * (vert2.position - v0);


    normal   = ( w * vert0.normal + 
                 u * vert1.normal +
                 v * vert2.normal ).direction();

    // Flip normal if on the backside
    if(backside){
        normal *= -1.0f;
    }

    texCoord = w * vert0.texCoord0 + 
               u * vert1.texCoord0 +
               v * vert2.texCoord0;
                 
}


void Tri::Intersector::getResult
(const CPUVertexArray& vertexArray,
 Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord,
 Vector3&        tangent1,
 Vector3&        tangent2) const {

    float w = 1.0 - u - v;
    // Portions are identical to the 3 parameter getResult, but copying code
    // to save pointer indirections on the vertex attributes
    const CPUVertexArray::Vertex& vert0 = tri->vertex(vertexArray, 0);
    const CPUVertexArray::Vertex& vert1 = tri->vertex(vertexArray, 1);
    const CPUVertexArray::Vertex& vert2 = tri->vertex(vertexArray, 2);

    const Point3& v0 = vert0.position;

    location = v0 + 
               u * (vert1.position - v0) +
               v * (vert2.position - v0);

    
    texCoord = w * vert0.texCoord0 + 
               u * vert1.texCoord0 +
               v * vert2.texCoord0;

    normal   = ( w * vert0.normal + 
                 u * vert1.normal +
                 v * vert2.normal ).direction();

    if (vert0.tangent.w == 0.0f) {
        tangent1 = Vector3::zero();
        tangent2 = Vector3::zero();
    } else {
        tangent1  = ( w * vert0.tangent.xyz() + 
                      u * vert1.tangent.xyz() +
                      v * vert2.tangent.xyz() ).direction();

        tangent2 = normal.cross(tangent1);
    }

    if(backside){
        normal   *= -1.0f;
        tangent1 *= -1.0f; 
        tangent2 *= -1.0f; 
    }

}


#ifdef _MSC_VER
// Turn off fast floating-point optimizations
#pragma float_control( pop )

#endif



} // namespace G3D
