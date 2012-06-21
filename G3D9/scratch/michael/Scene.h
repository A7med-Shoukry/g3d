#ifndef Scene_h
#define Scene_h

#include <G3D/G3DAll.h>
#include "Entity.h"


/** Sample scene graph.

    Includes loading from a text file, and a GUI component for detecting
    and selecting scenes.

    G3D does not contain a Scene class in the API because it is a
    very application-specific role. This is a sample of how you 
    might begin to structure one to get you started.
*/
class Scene : public ReferenceCountedObject {
protected:
    /** Current time */
    GameTime                    m_time;
    Lighting::Ref               m_lighting;
    Texture::Ref                m_skyBoxTexture;
    float                       m_skyBoxConstant;
    Array<Entity::Ref>          m_entityArray;

    Scene() : 
        m_time(0), 
        m_skyBoxTexture(Texture::whiteCube()), 
        m_skyBoxConstant(1.0f) {}

public:

    typedef ReferenceCountedPointer<Scene> Ref;

    static Scene::Ref create(const std::string& sceneName, GCamera& camera);
    
    virtual void onPose(Array<Surface::Ref>& surfaceArray);

    virtual void onSimulation(GameTime deltaTime);

    Lighting::Ref lighting() const {
        return m_lighting;
    }

    GameTime time() const {
        return m_time;
    }

    Texture::Ref skyBoxTexture() const {
        return m_skyBoxTexture;
    }

    float skyBoxConstant() const {
        return m_skyBoxConstant;
    }

    /** Enumerate the names of all available scenes. */
    static Array<std::string> sceneNames();

	/** Returns the Entity whose conservative bounds are first
	    intersected by \a ray, excluding Entitys in \a exclude.  
		Useful for mouse selection.  Returns NULL if none are intersected.

		Note that this may not return the closest Entity if another's bounds
		project in front of it.
     */	  
	Entity::Ref intersectBounds(const Ray& ray, const Array<Entity::Ref>& exclude = Array<Entity::Ref>());
};

#endif
