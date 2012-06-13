#include "Entity.h"


void Entity::setMD3Animation(MD3Model::PartType partType, MD3Model::AnimType animType){
     m_md3Pose.anim[partType] = animType;
}

Entity::Entity
(const std::string& name,
 AnyTableReader&    propertyTable,
 const ModelTable&  modelTable) : 
    GEntity(name, propertyTable, modelTable) {
}


Entity::Ref Entity::create
(const std::string& name,
 AnyTableReader&    propertyTable,
 const ModelTable&  modelTable) {

    return new Entity(name, propertyTable, modelTable);
}
