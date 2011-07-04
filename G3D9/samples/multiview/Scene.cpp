#include "Scene.h"

using namespace G3D::units;

Entity::Entity() {}

Entity::Ref Entity::create(const CFrame& c, const ArticulatedModel::Ref& m) {
    Ref e = new Entity();
    e->frame = c;
    e->model = m;
    return e;
}


void Entity::onPose(Array<Surface::Ref>& surfaceArray) {
    model->pose(surfaceArray, frame);
}


Scene::Ref Scene::create() {
    Scene::Ref s = new Scene();
    s->m_lighting = GApp::defaultLighting();

    ArticulatedModel::Ref model = ArticulatedModel::fromFile(System::findDataFile("models/dabrovic_sibenik/sibenik.obj"));
    s->m_entityArray.append(Entity::create(Point3::zero(), model));

    return s;
}
    

void Scene::onPose(Array<Surface::Ref>& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}
