#include "Entity.h"

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

    Entity::Ref e = new Entity(name, propertyTable, modelTable);
    // Set the initial position
    e->onSimulation(0, 0);
    return e;
}
