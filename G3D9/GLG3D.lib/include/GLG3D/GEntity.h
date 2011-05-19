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

    /** The Any from which this was originally constructed */
    Any                             m_sourceAny;

    /** Root position over time.  Set m_frameSplineChanged if this
        changes. */
    PhysicsFrameSpline              m_frameSpline;

    /** True if the spline was mutated since load.  Used by toAny() to
        decide if m_sourceAny is out of date. */
    bool                            m_frameSplineChanged;

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

    //////////////////////////////////////////////

    /** Bounds at the last pose() call, in world space. */ 
    AABox                           m_lastBoxBounds;

    /** Bounds at the last pose() call, in world space. */ 
    Sphere                          m_lastSphereBounds;

    GEntity();

    /** \deprecated */
    GEntity
    (const std::string& n, 
     const PhysicsFrameSpline& frameSpline, 
     const ArticulatedModel::Ref& artModel,
     const ArticulatedModel::PoseSpline& artPoseSpline,
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

    /** Current position, i.e., as of last onSimulation call */
    const CFrame& frame() const {
        return m_frame;
    }

    const std::string& name() const {
        return m_name;
    }

    /** Provides access to the underlying spline */
    const PhysicsFrameSpline& frameSpline() const {
        return m_frameSpline;
    }

    void setFrameSpline(const PhysicsFrameSpline& s) {
        if (m_frameSpline != s) {
            m_frameSplineChanged = true;
        }
        m_frameSpline = s;
    }

    /** Converts the current GEntity to an Any.  Subclasses should
        modify at least the name of the Table, which will be "GEntity"
        if not changed. */
    virtual Any toAny() const;

    /** 
        \brief Physical simulation callback.

        The default implementation animates the model pose (by calling
        simulatePose()) and moves the frame() along m_frameSpline.
     */
    virtual void onSimulation(GameTime absoluteTime, GameTime deltaTime);

    /** Pose as of the last simulation time */
    virtual void onPose(Array<Surface::Ref>& surfaceArray);

    /** Return a world-space axis-aligned bounding box as of the last call to onPose(). */
    virtual void getLastBounds(class AABox& box) const;

    /** Return a world-space bounding sphere as of the last call to onPose(). */
    virtual void getLastBounds(class Sphere& sphere) const;
    
    /** Return a world-space bounding box as of the last call to onPose(). */
    virtual void getLastBounds(class Box& box) const;

    /** Returns true if there is conservatively some intersection
        with the object's bounds closer than \a maxDistance to the
        ray origin.  If so, updates maxDistance with the intersection distance.
        
        The bounds used may be more accurate than any of the given
        getLastBounds() results because the method may recurse into
        individual parts of the scene graph within the GEntity. */
    virtual bool intersectBounds(const Ray& R, float& maxDistance) const;

    virtual bool intersect(const Ray& R, float& maxDistance) const;
};

}
#endif
