#include "GLG3D/GEntity.h"
#include "G3D/Box.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "G3D/Ray.h"

namespace G3D {

GEntity::GEntity() {}

GEntity::GEntity
(const std::string& name,
 AnyTableReader&    propertyTable,
 const ModelTable&  modelTable) 
    : m_name(name),
      m_modelType(ARTICULATED_MODEL) {

    const Any& modelNameAny = propertyTable["model"];
    const std::string& modelName = modelNameAny.string();
    
    const ReferenceCountedPointer<ReferenceCountedObject>* model = modelTable.getPointer(modelName);
    modelNameAny.verify((model != NULL), 
                        "Can't instantiate undefined model named " + modelName + ".");
    
    m_artModel = model->downcast<ArticulatedModel>();
    m_md2Model = model->downcast<MD2Model>();
    m_md3Model = model->downcast<MD3Model>();

    if (m_artModel.notNull()) {
        m_modelType = ARTICULATED_MODEL;
        propertyTable.getIfPresent("pose", m_artPoseSpline);
    } else if (m_md2Model.notNull()) {
        m_modelType = MD2_MODEL;
    } else if (m_md3Model.notNull()) {
        m_modelType = MD3_MODEL;
    }

    if (! propertyTable.getIfPresent("position", m_frameSpline)) {
        // Create a default value
        m_frameSpline.append(CFrame());
    }
}


GEntity::GEntity
(const std::string& n, const PhysicsFrameSpline& frameSpline, 
const ArticulatedModel::Ref& artModel, const ArticulatedModel::PoseSpline& artPoseSpline,
const MD2Model::Ref& md2Model,
const MD3Model::Ref& md3Model) : 
    m_name(n), 
    m_modelType(ARTICULATED_MODEL),
    m_frameSpline(frameSpline),
    m_artPoseSpline(artPoseSpline), 
    m_artModel(artModel),
    m_md2Model(md2Model), m_md3Model(md3Model) {

    m_name  = n;
    m_frameSpline = frameSpline;

    if (artModel.notNull()) {
        m_modelType = ARTICULATED_MODEL;
    } else if (md2Model.notNull()) {
        m_modelType = MD2_MODEL;
    } else if (md3Model.notNull()) {
        m_modelType = MD3_MODEL;
    }
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::Ref& m, const ArticulatedModel::PoseSpline& poseSpline) {
    GEntity::Ref e = new GEntity(n, frameSpline, m, poseSpline, NULL, NULL);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD2Model::Ref& m) {
    GEntity::Ref e = new GEntity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), m, NULL);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


GEntity::Ref GEntity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD3Model::Ref& m) {
    GEntity::Ref e = new GEntity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), NULL, m);

    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}


void GEntity::simulatePose(GameTime absoluteTime, GameTime deltaTime) {
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artPoseSpline.get(float(absoluteTime), m_artPose);
        break;

    case MD2_MODEL:
        {
            MD2Model::Pose::Action a;
            m_md2Pose.onSimulation(deltaTime, a);
            break;
        }

    case MD3_MODEL:
        m_md3Model->simulatePose(m_md3Pose, deltaTime);
        break;
    }
}


void GEntity::onSimulation(GameTime absoluteTime, GameTime deltaTime) {
    m_frame = m_frameSpline.evaluate(float(absoluteTime));

    simulatePose(absoluteTime, deltaTime);
}


void GEntity::onPose(Array<Surface::Ref>& surfaceArray) {
    const int oldLen = surfaceArray.size();

    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artModel->pose(surfaceArray, m_frame, m_artPose);
        break;

    case MD2_MODEL:
        m_md2Model->pose(surfaceArray, m_frame, m_md2Pose);
        break;

    case MD3_MODEL:
        m_md3Model->pose(surfaceArray, m_frame, m_md3Pose);
        break;
    }

    // Compute bounds
    m_lastBoxBounds = AABox::empty();
    m_lastSphereBounds = Sphere(m_frame.translation, 0);
    for (int i = oldLen; i < surfaceArray.size(); ++i) {
        AABox b;
        Sphere s;
        const Surface::Ref& surf = surfaceArray[i];
        
        surf->getWorldSpaceBoundingBox(b);
        surf->getWorldSpaceBoundingSphere(s);
        m_lastBoxBounds.merge(b);
        m_lastSphereBounds.radius = max(m_lastSphereBounds.radius,
                                        (s.center - m_lastSphereBounds.center).length() + s.radius);
    }
}


void GEntity::getLastBounds(AABox& box) const {
    box = m_lastBoxBounds;
}


void GEntity::getLastBounds(Box& box) const {
    box = m_lastBoxBounds;
}


void GEntity::getLastBounds(Sphere& sphere) const {
    sphere = m_lastSphereBounds;
}


bool GEntity::intersectBounds(const Ray& R, float& maxDistance) const {
    float t = R.intersectionTime(m_lastBoxBounds);
    if (t < maxDistance) {
        maxDistance = t;
        return true;
    } else {
        return false;
    }
}


bool GEntity::intersect(const Ray& R, float& maxDistance) const {
    // TODO
    return false;
}

}
