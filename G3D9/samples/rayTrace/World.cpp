#include "World.h"

World::World() : m_mode(TRACE) {
    begin();

    lightArray.append(GLight::point(Vector3(0, 10, 0), Color3::white() * 1200));
    lightArray.append(GLight::point(Vector3(22.6f, 2.9f,  6.6f), Color3::fromARGB(0xffe5bd) * 1000));

    ambient = Radiance3::fromARGB(0x304855) * 0.8f;

    Any teapot = PARSE_ANY
        ( ArticulatedModel2::Specification {
            filename = "teapot/teapot.obj";
            scale = 0.01;
            stripMaterials = true;
            preprocess = 
                ( setMaterial(all(), all(),
                             Material::Specification {
                                 specular = Color3(0.2f);
                                 shininess = mirror();
                                 lambertian = Color3(0.7f, 0.5f, 0.1f);
                             });
                 );
         } );

    insert(ArticulatedModel2::create(teapot), CFrame::fromXYZYPRDegrees(19.4f, -0.2f, 0.94f, 70));

    Any sphereOutside = PARSE_ANY 
        ( ArticulatedModel2::Specification {
            filename = "sphere.ifs";
            scale = 0.3;
            preprocess =
                ( setMaterial(all(), all(),
                              Material::Specification {
                                  specular = Color3(0.1f);
                                  shininess = mirror();
                                  lambertian = Color3(0.0f);
                                  etaTransmit = 1.3f;
                                  etaReflect = 1.0f;
                                  transmissive = Color3(0.2f, 0.5f, 0.7f);
                              });
                  );
          });

    Any sphereInside = PARSE_ANY 
        ( ArticulatedModel2::Specification {
            filename = "sphere.ifs";
            scale = -0.3;
            preprocess =
                ( setMaterial(all(), all(),
                              Material::Specification {
                                  specular = Color3(0.1f);
                                  shininess = mirror();
                                  lambertian = Color3(0.0f);
                                  etaReflect = 1.3f;
                                  etaTransmit = 1.0f;
                                  transmissive = Color3(1.0f);
                              });
                  );
          });

    insert(ArticulatedModel2::create(sphereOutside), CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));
    insert(ArticulatedModel2::create(sphereInside),  CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));

    Any sponza = PARSE_ANY
        ( ArticulatedModel2::Specification {
            filename = "dabrovic_sponza/sponza.zip/sponza.obj";
          } );
    insert(ArticulatedModel2::create(sponza), Vector3(8.2f, -6, 0));
    
    end();
}


void World::begin() {
    debugAssert(m_mode == TRACE);
    m_surfaceArray.clear();
    m_triArray.clear();
    m_mode = INSERT;
}


void World::insert(const ArticulatedModel2::Ref& model, const CFrame& frame) {
    Array<Surface::Ref> posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}


void World::insert(const ArticulatedModel::Ref& model, const CFrame& frame) {
    Array<Surface::Ref> posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}

void World::insert(const Surface::Ref& m) {
    debugAssert(m_mode == INSERT);
    m_surfaceArray.append(m);
    Tri::getTris(m, m_triArray, CFrame());
}


void World::end() {
    for (int i = 0; i < m_triArray.size(); ++i) {
        m_triArray[i].material()->setStorage(MOVE_TO_CPU);
    }

    debugAssert(m_mode == INSERT);
    m_mode = TRACE;

    TriTree::Settings s;
    s.algorithm = TriTree::MEAN_EXTENT;
    m_triTree.setContents(m_triArray, s);
    m_triArray.clear();
}


bool World::lineOfSight(const Vector3& v0, const Vector3& v1) const {
    debugAssert(m_mode == TRACE);
    
    Vector3 d = v1 - v0;
    float len = d.length();
    Ray ray = Ray::fromOriginAndDirection(v0, d / len);
    float distance = len;
    Tri::Intersector intersector;

    return ! m_triTree.intersectRay(ray, intersector, distance, true);

}


bool World::intersect(const Ray& ray, float& distance, SurfaceElement& surfel) const {
    debugAssert(m_mode == TRACE);

    Tri::Intersector intersector;
    if (m_triTree.intersectRay(ray, intersector, distance)) {
        surfel = SurfaceElement(intersector);
        return true;
    } else {
        return false;
    }
}
