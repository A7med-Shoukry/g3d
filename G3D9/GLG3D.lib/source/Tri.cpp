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
		CPUVertexArray* vertexArray,
        const Proxy<Material>::Ref& material) :
    m_material(material) {
    

    debugAssert(material.isNull() ||
                (isValidHeapPointer(material.pointer()) &&
                 (material.pointer() > (void*)0xf)));
	index[0] = i0;
	index[1] = i1;
	index[2] = i2;

	m_cpuVertexArray = vertexArray;
	
	v0                = m_cpuVertexArray->vertex[i0].position;
	const Vector3& v1 = m_cpuVertexArray->vertex[i1].position;
	const Vector3& v2 = m_cpuVertexArray->vertex[i2].position;

	e1 = v1 - v0;
	e2 = v2 - v0;

    m_doubleArea = e1.cross(e2).length();
}


Tri::operator Triangle() const {
    return Triangle(position(0), position(1), position(2));
}
    

Tri Tri::otherSide() const {
    Tri t;
    
    t.m_material = m_material;
    t.v0     = v0;
    t.e1     = e2;
    t.e2     = e1;
	t.m_doubleArea = m_doubleArea;


	t.m_cpuVertexArray = m_cpuVertexArray;
    // Flip winding 
    // by swapping elements 1 and 2.
	// normal/tangents remain the same
    

    t.index[0]   = index[0];
    

    for (int i = 1; i < 3; ++i) {
        int j = 3 - i;
        t.index[i]   = index[j];
    }
    
    return t;
}


#ifdef _MSC_VER
// Turn on fast floating-point optimizations
#pragma float_control( push )
#pragma fp_contract( on )
#pragma fenv_access( off )
#pragma float_control( except, off )
#pragma float_control( precise, off )
#endif

bool Tri::Intersector::operator()(const Ray& ray, const Tri& tri, bool twoSided, float& distance) {
    // See RTR3 p.746 (RTR2 ch. 13.7) for the basic algorithm.
    static const float EPS = 1e-12f;

    // How much to grow the edges of triangles by to allow for small roundoff.
    static const float conservative = 1e-8f;

    

	const Vector3& e1 = tri.e1;
    const Vector3& e2 = tri.e2;


	// Test for backfaces first because this eliminates 50% of all triangles.

	// This test is equivalent to n.dot(ray.direction()) >= -EPS
	// Where n is the face unit normal, which we do not explicitly store
	
	if (!twoSided && (e1.cross(e2)).dot(ray.direction()) >= -EPS * tri.m_doubleArea) {
        // Backface or nearly parallel
        return false;
    }

    const Vector3& v0 = tri.v0;
 

    const Vector3& p = ray.direction().cross(e2);

	// Will be negative if we are coming from the back.
    const float a = e1.dot(p);

    //debugAssert(a >= -1e-7); 

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
                        w * tri.texCoord(0) + 
                        u * tri.texCoord(1) +
                        v * tri.texCoord(2);
            
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
        // location, but don't bother computing barycentric w,
        // the hit location or the normal until after we've checked
        // against all triangles.

        distance = t;
        this->tri = &tri;

		eye = ray.direction();
		this->u = u;
		this->v = v;
        return true;
    } else {
        return false;
    }
}


void Tri::Intersector::getResult
(Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord) const {

    debugAssert(tri);
    float w = 1.0 - u - v;

    location = tri->v0 + 
               u * tri->e1 +
               v * tri->e2;

	
	const CPUVertexArray::Vertex& v0 = tri->vertex(0);
	const CPUVertexArray::Vertex& v1 = tri->vertex(1);
	const CPUVertexArray::Vertex& v2 = tri->vertex(2);
	
    texCoord = w * v0.texCoord0 + 
               u * v1.texCoord0 +
               v * v2.texCoord0;

    normal   = ( w * v0.normal + 
                 u * v1.normal +
                 v * v2.normal ).direction();
				 
}


void Tri::Intersector::getResult
(Vector3&        location,
 Vector3&        normal,
 Vector2&        texCoord,
 Vector3&        tangent1,
 Vector3&        tangent2) const {

    float w = 1.0 - u - v;
	// Identical to the parameter getResult, but copying code
	// to save pointer indirections on the vertex attributes
	location = tri->v0 + 
               u * tri->e1 +
               v * tri->e2;

	const CPUVertexArray::Vertex& v0 = tri->vertex(0);
	const CPUVertexArray::Vertex& v1 = tri->vertex(1);
	const CPUVertexArray::Vertex& v2 = tri->vertex(2);

    texCoord = w * v0.texCoord0 + 
               u * v1.texCoord0 +
               v * v2.texCoord0;

    normal   = ( w * v0.normal + 
                 u * v1.normal +
                 v * v2.normal ).direction();

    if (v0.tangent.w == 0.0f) {
        tangent1 = Vector3::zero();
        tangent2 = Vector3::zero();
    } else {
        tangent1  = ( w * v0.tangent.xyz() + 
                      u * v1.tangent.xyz() +
                      v * v2.tangent.xyz() ).direction();

        tangent2 = normal.cross(tangent1);
    }
}


#ifdef _MSC_VER
// Turn off fast floating-point optimizations
#pragma float_control( pop )

#endif



} // namespace G3D
