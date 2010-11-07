#ifndef GLG3D_GEntity_H
#define GLG3D_GEntity_H

#include "G3D/CoordinateFrame.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/MD2Model.h"
#include "GLG3D/MD3Model.h"

namespace G3D {

/** \brief Sample base class for an object in a 3D world.

    G3D does not contain a mandatory Entity class in the API because
    it is a very application-specific role. However, this is a base
    class of how you might begin to structure one to get you started.

    \beta
*/
class GEntity : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<GEntity> Ref;

    /** Maps model names that are referenced in to ArticulatedModel::Ref, MD2Model::Ref or MD3::ModelRef */
    typedef Table<std::string, ReferenceCountedPointer<ReferenceCountedObject> > ModelTable;

protected:

    enum ModelType {
        ARTICULATED_MODEL,
        MD2_MODEL,
        MD3_MODEL
    };

    std::string                     m_name;

    ModelType                       m_modelType;

    /** Current position */
    CFrame                          m_frame;

    /** Root position over time */
    PhysicsFrameSpline              m_frameSpline;

    //////////////////////////////////////////////

    /** Current pose */
    ArticulatedModel::Pose          m_artPose;

    /** Pose over time. */
    ArticulatedModel::PoseSpline    m_artPoseSpline;

    ArticulatedModel::Ref           m_artModel;

    //////////////////////////////////////////////

    MD2Model::Ref                   m_md2Model;
    MD2Model::Pose                  m_md2Pose;

    //////////////////////////////////////////////

    MD3Model::Ref                   m_md3Model;
    MD3Model::Pose                  m_md3Pose;

    GEntity();

    /** \deprecated */
    GEntity(const std::string& n, const PhysicsFrameSpline& frameSpline, 
        const ArticulatedModel::Ref& artModel, const ArticulatedModel::PoseSpline& artPoseSpline,
        const MD2Model::Ref& md2Model,
        const MD3Model::Ref& md3Model);


    /**\brief Construct a GEntity.

       \param name The name of this GEntity, e.g., "Player 1"

       \param propertyTable The form is given below.  It is intended that
       subclasses replace the table name and add new fields.

       \code
       <some base class name> {
           model    = <modelname>,
           position = <CFrame, Vector3, or PhysicsFrameSpline>,
           pose     = <ArticulatedModel::PoseSpline>
       }
       \endcode

       The pose and position fields are optional.  The GEntity base class
       reads these fields.  Other subclasses read their own fields.

       \param modelTable Maps model names that are referenced in \a propertyTable
       to ArticulatedModel::Ref, MD2Model::Ref or MD3::ModelRef.
       
       The original caller (typically, a Scene class) should invoke
       AnyTableReader::verifyDone to ensure that all of the fields 
       specified were read by some subclass along the inheritance chain.       

       See samples/starter/source/Scene.cpp for an example of use.
     */
    GEntity
    (const std::string& name,
     AnyTableReader&    propertyTable,
     const ModelTable&  modelTable);

    /** Animates the appropriate pose type for the model selected.
     Called from onSimulation.  Subclasses will frequently replace
     onSimulation but retain this helper method.*/
    virtual void simulatePose(GameTime absoluteTime, GameTime deltaTime);

public:

    /** \deprecated */
    static GEntity::Ref create(const std::string& n, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::Ref& m, const ArticulatedModel::PoseSpline& poseSpline);

    /** \deprecated */
    static GEntity::Ref create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD2Model::Ref& m);

    /** \deprecated */
    static GEntity::Ref create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD3Model::Ref& m);

    /** \copydoc GEntity(const std::string&, AnyTableReader&, const ModelTable&)*/
    static GEntity::Ref create 
    (const std::string& name,
     AnyTableReader&    propertyTable,
     const ModelTable&  modelTable) {
        return new GEntity(name, propertyTable, modelTable);
    }

    const CFrame& frame() const {
        return m_frame;
    }

    const std::string& name() const {
        return m_name;
    }

    /** 
        \brief Physical simulation callback.

        The default implementation animates the model pose (by calling
        simulatePose()) and moves the frame() along m_frameSpline.
     */
    virtual void onSimulation(GameTime absoluteTime, GameTime deltaTime);

    /** Pose as of the last simulation time */
    virtual void onPose(Array<Surface::Ref>& surfaceArray);
#if 0
    /** Return a world-space axis-aligned bounding box. */
    virtual void getBounds(class AABox& box) const;

    /** Return a world-space bounding sphere. */
    virtual void getBounds(class Sphere& sphere) const;

    /** Return a world-space bounding box. */
    virtual void getBounds(class Box& box) const;

    /** Return the distance to the first intersection of the GEntity's bounds
        with ray \a R at distance
        less than \a maxDistance.  Returns finf() if there is no such intersection.
        
        The bounds used may be more accurate than any of the given getBounds() results
        because the method may recurse into individual parts of the scene graph
        within the GEntity.
    */
    float intersectBounds(const Ray& R, float maxDistance = finf()) const;
#endif
};

}
#endif
