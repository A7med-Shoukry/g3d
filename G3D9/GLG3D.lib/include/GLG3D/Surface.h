/**
  \file GLG3D/Surface.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-11-15
  \edited  2011-06-09
 */ 

#ifndef GLG3D_Surface_h
#define GLG3D_Surface_h

#include "G3D/Array.h"
#include "G3D/Color4.h"
#include "G3D/units.h"
#include "GLG3D/Texture.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/GBuffer.h"

namespace G3D {

class ShadowMap;

namespace SuperShader {
class Pass;
typedef ReferenceCountedPointer<Pass> PassRef;
}

/** 
  Simple material used by IFSModel and MD2Model pose methods.
  This class is provided as a convenience; it is not necessary
  to use it.  If you do not provide a material, whatever
  state is currently on the G3D::RenderDevice is used.  
  
  This is a legacy class for fixed function rendering. You
  probably don't want to use GMaterial at all if you
  are writing vertex and pixel shaders.

  @deprecated
  @sa G3D::Material
 */
class GMaterial {
public:
    float                   specularCoefficient;
    float                   shininess;
    /** Diffuse color */
    Color4                  color;
    Array<Texture::Ref>     texture;

    inline GMaterial(TextureRef t = NULL) : specularCoefficient(0.2f), shininess(10), color(Color3::white()) {
        if (t.notNull()) {
            texture.append(t);
        }
    }

    /** Applies this material to the render device */
    void configure(class RenderDevice* rd) const;
};

typedef ReferenceCountedPointer<class Surface> SurfaceRef;


/**
   \brief The surface of a model, posed and ready for rendering.

   Many subclasses of Surface need to bind shader and other state in
   order to render.  To amortize the cost of doing so, renderers use
   categorizeByDerivedType<Surface::Ref> to distinguish subclasses and
   then invoke the methods with names ending in "Homogeneous" on
   arrays of derived instances.

   Most methods take a \a timeOffset argument.  This is the time in
   seconds to offset the result from the time at which the model was
   posed.  The location of the rendered object is only an
   approximation when this value is non-zero.  For most Surface
   subclasses, small negative offsets produce fairly accurate
   positioning because the object can be interpolated from the
   previous pose-time state.  Positive offsets lead to extrapolation
   and are often less accurate.  Note that one could also render at
   multiple times by posing the original models at different times.
   However, models do not guarantee that they will produce the same
   number of Surface%s, or Surface%s with the same topology each time
   that they are posed.  The use of timeOffset allows the caller to
   assume that the geometry deforms but has the same topology across
   an interval.
 */
class Surface : public ReferenceCountedObject {
protected:

    Surface() {}

public:

    typedef ReferenceCountedPointer<class Surface> Ref;

    /** \brief How sortAndRender() configures the RenderDevice to
        process alpha */
    enum AlphaMode {
        /** Alpha > 0.5 is rendered, alpha <= 0.5 is discarded. */
        ALPHA_BINARY,

        /** Convert alpha to coverage values using
           <code>glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB)</code>.
           Requires a MSAA framebuffer to be bound.*/
        // See http://www.dhpoware.com/samples/glMultiSampleAntiAliasing.html for an example.
        ALPHA_TO_COVERAGE,
        
        /** Render surfaces with partial coverage from back to front,
            using Porter and Duff's OVER operator.  This leaves the
            depth buffer inconsistent with the color buffer and
            requires a sort, but often gives the best appearance. */
        ALPHA_BLEND
    };

    virtual ~Surface() {}

    virtual std::string name() const = 0;

    virtual void getCoordinateFrame(CoordinateFrame& cframe, float timeOffset = 0.0f) const = 0;

    virtual void getObjectSpaceBoundingBox(AABox& box, float timeOffset = 0.0f) const = 0;

    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, float timeOffset = 0.0f) const = 0;


    /** \brief Clears the arrays and appends indexed triangle list
        information.
    
     Most subclasses will ignore \a timeOffset because they only use
     that for rigid-body transformations.  However, it is possible to
     include skinning or keyframe information in a Surface and respond
     to timeOffset.
    
     Not required to be implemented.*/
    virtual void getObjectSpaceGeometry
    (Array<int>&                  index, 
     Array<Point3>&               vertex, 
     Array<Vector3>&              normal, 
     Array<Vector4>&              packedTangent, 
     Array<Point2>&               texCoord, 
     float                        timeOffset = 0.0f) {}


    /** If true, this object transmits light and depends on
        back-to-front rendering order and should be rendered in sorted
        order. 

        The default implementation returns false.*/
    virtual bool hasTransmission() const {
        return false;
    }


    /** If true, this object's material produces subpixel coverage
        (i.e. alpha) and may require back-to-front rendering depending
        on Surface::AlphaMode. 

        The default implementation returns false.*/
    virtual bool hasPartialCoverage() const {
        return false;
    }


    /** A hint to the renderer indicating that this surface should
        write to the depth buffer.  Typically overridden to return
        false for surfaces with very low partial coverage (alpha) or
        transmission values, or to resolve artifacts for specific
        scenes.  The default value is ! hasTransmission().*/
    virtual bool depthWriteHint(float distanceToCamera) const {
        (void)distanceToCamera;
        return ! hasTransmission();
    }

    ///////////////////////////////////////////////////////////////////////
    // Aggregate methods

    /** \brief Per-frame lighting information passed to renderHomogeneous. */
    class Environment {
    protected:

        Array<ShadowMap::Ref> m_shadowMapArray;

        Texture::Ref          m_sourceScreenColorTexture;
        Texture::Ref          m_sourceScreenDepthTexture;

        /** True if the screen textures have been copied since the
            last call to setData. Set to true in screenTexture() and
            screenDepthTexture(). */ 
        mutable bool          m_copiedScreenSinceLastSetData;
        Texture::Ref          m_copiedScreenColorTexture;
        Texture::Ref          m_copiedScreenDepthTexture;
        
    public:

        Environment() {}
        
        void setData
        (const Lighting::Ref&         lighting,
         const Array<ShadowMap::Ref>& shadowMapArray,
         const Texture::Ref&          sourceScreenColorTexture, 
         const Texture::Ref&          sourceScreenDepthTexture);

        virtual ~Environment() {}

        const Lighting::Ref& lighting() const;

        /** An array of the same length as lighting->lightArray whose
            elements are NULL for non-shadow casting lights and
            contain shadow maps that have already been rendered for
            the shadow casting lights.
            */
        const Array<ShadowMap::Ref>& shadowMapArray() const;

        /** An image of the color buffer.  This is a copy of the
            previous buffer; it is never the Texture currently being
            rendered to.

            Commonly used for screen-space reflection and refraction
            effects. This may be lazily computed on the first call.

            \param updateNow If true, the texture is re-copied from
            the source to include objects that have been rendered
            since the previous call.  If false, the implementation may
            choose whether to re-copy the data.  The default
            implementation does not re-copy the texture unless
            this is the first call.
        */
        virtual const Texture::Ref& screenColorTexture(bool updateNow) const;

        /** Used for screen-space reflection and refraction
            effects. 

            \sa screenColorTexture
        */
        virtual const Texture::Ref& screenDepthTexture(bool updateNow) const;

        /** More efficient than calling screenColorTexture and
            screenDepthTexture independently when both are needed. */
        virtual void getScreenTextures(Texture::Ref& color, Texture::Ref& depth, bool updateNow) const;
    };

    /**
     \brief Forward-render all illumination terms for each element of
     \a surfaceArray, which must all be of the same most-derived type
     as \a this.

     Rendering proceeds in the order of elements in the surfaceArray.
     The caller may sort the array first to create back-to-front or
     front-to-back ordering.  To improve the rendering of 

     Implementations must obey the semantics of the current stencil,
     viewport, clipping, and depth tests.

     Elements may have partial coverage but are assumed to not have
     transmission.

     Invoking this with elements of \a surfaceArray that are not of
     the same most-derived type as \a this will result in an error.

     \param timeOffset All lighting occurs at timeOffset = 0, but
     object positions should be moved to respect the timeOffset.

     \param environment World-space, screen-space, and light-space
     data needed for illumination.    
     */
    virtual void renderHomogeneous
    (RenderDevice*                rd, 
     const Array<Surface::Ref>&   surfaceArray, 
     const Environment&           environment,
     float                        timeOffset = 0.0f) const {}//= 0;


    /** 
    \brief Render all instances of \a surfaceArray to the
    currently-bound Framebuffer using the fields and mapping dictated
    by \a specification.  This is also used for depth-only (e.g.,
    z-prepass) rendering.

    Invoking this with elements of \a surfaceArray that are not of the
    same most-derived type as \a this will result in an error.

    \param velocityStartOffset Time at which the previous frame should
    be sampled when computing the GBuffer velocity buffer.  Set to
    -1/framerate if performing reverse reprojection using velocity
    buffers.
    */
    virtual void renderGBufferHomogeneous
    (RenderDevice*                rd, 
     Array<Surface::Ref>&         surfaceArray,
     const GBuffer::Specification& specification,
     float                        timeOffset = 0.0f,
     float                        velocityStartOffset = -1.0f) const {}//= 0;


    /** \brief Rendering a set of surfaces in wireframe, using the
       current blending mode.  This is primarily used for debugging.
       
       Invoking this with elements of \a surfaceArray that are not of
       the same most-derived type as \a this will result in an error.
       */
    virtual void renderWireframeHomogeneous
    (RenderDevice*                rd, 
     const Array<Surface::Ref>&   surfaceArray, 
     const Color4&                color = Color3::black(), 
     float                        timeOffset = 0.0f) const {}//= 0;

    ///////////////////////////////////////////////////////////////////////
    // Static methods

    /** 
      Divides the inModels into a front-to-back sorted array of opaque
      models and a back-to-front sorted array of potentially
      transparent models.  Any data originally in the output arrays is
      cleared.

      \param wsLookVector Sort axis; usually the -Z axis of the camera.
     */
    static void sortFrontToBack
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector,
        float                      timeOffset = 0.0f);


    static void sortBackToFront
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector,
        float                      timeOffset = 0.0f) {
        sortFrontToBack(surfaces, -wsLookVector, timeOffset);
    }


    /** Utility function for rendering a set of surfaces in wireframe using the current blending mode. */
    static void renderWireframe(RenderDevice* rd, const Array<Surface::Ref>& models, const Color4& color = Color3::black(), float timeOffset = 0.0f);

    /** Computes the world-space bounding box of an array of Surface%s of any type.*/
    static void getBoxBounds(const Array<Surface::Ref>& surfaceArray, AABox& bounds, float timeOffset = 0.0f);

    /** Computes the world-space bounding sphere of an array of Surface%s of any type.*/
    static void getSphereBounds(const Array<Surface::Ref>& surfaceArray, Sphere& bounds, float timeOffset = 0.0f);

    /** Computes the array of models that can be seen by \a camera*/
    static void cull(const class GCamera& camera, const class Rect2D& viewport, const Array<Surface::Ref>& allModels, 
                     Array<Surface::Ref>& outModels, float timeOffset = 0.0f);

    /**
     Removes elements from \a all and puts them in \a translucent.
     \a translucent is cleared first.
     Always treats hasTransmissive() objects as translucent.
     If \a partialCoverageIsTranslucent is true, also treats hasPartialCoverage as translucent.
     */
    static void extractTranslucent(Array<Surface::Ref>& all, Array<Surface::Ref>& translucent, bool partialCoverageIsTranslucent);

    ///////////////////////////////////////////////////////////////////////
    // Deprecated

    /** Render using current fixed function lighting environment. Do
        not change the current state. Behavior with regard to stencil,
        shadowing, etc. is intentionally undefinded.

        Default implementation calls defaultRender.

        \deprecated
    */
    virtual void render(class RenderDevice* renderDevice) const;

    /** 
     Render all terms that are independent of shadowing (e.g.,
     transparency, reflection, ambient illumination, emissive
     illumination, nonShadowCastingLights). Transparent objects are
     assumed to render additively (but should set the blend mode
     themselves). Restore all state to the original form on exit.
     Default implementation invokes render.

     Implementation must obey the current stencil, depth write, color
     write, and depth test modes.  Implementation may freely set the
     blending, and alpha test modes.

     The caller should invoke this in depth sorted back to front order
     for transparent objects.

     The default implementation configures the non-shadow casting
     lights and calls render.

     Implementation advice:
      <UL>
        <LI> If color write is disabled, don't bother performing any shading on this object.
        <LI> It may be convenient to support multiple lights by invoking renderShadowedLightPass multiple times.
      </UL>

     The implementation must ignore shadow casting lights from \a lighting.

        \deprecated
    */
    virtual void renderNonShadowed(
        RenderDevice* rd,
        const LightingRef& lighting) const;

    /** Render illumination from this source additively, held out by the shadow map (which the caller 
        must have computed, probably using renderNonShadowed).  Default implementation
        configures the shadow map in texture unit 1 and calls render. 

        \deprecated
        */
    virtual void renderShadowMappedLightPass(
        RenderDevice* rd, 
        const GLight& light,
        const ReferenceCountedPointer<ShadowMap>& shadowMap) const;


    /**
     Configures the SuperShader with the G3D::Material for this object
     and renders it.  If this object does not support G3D::Materials
     (or an equivalent) may render nothing.  These passes will be additively blended
     with previous ones.

     @return True if state was preserved, false if the renderdevice is in a different state than when called.
    
     \param originalCullFace When rendering with an opposite
     winding direction (e.g., for mirrors), renderSuperShaderPass
     needs to be able to invert its culling direction.  Pass CULL_BACK
     to use "regular" culling and CULL_FRONT to invert the culling sense.
    @beta
     */
    virtual bool renderSuperShaderPass
    (RenderDevice* rd, 
     const SuperShader::PassRef& pass,
     RenderDevice::CullFace originalCullFace = RenderDevice::CULL_BACK) const {
        (void) rd;
        (void) pass;
        return true;
    }


    /** @deprecated */
    virtual void renderShadowMappedLightPass
    (RenderDevice* rd, 
     const GLight& light,
     const Matrix4& lightMVP,
     const Texture::Ref& shadowMap) const;

    /**
      Sends all geometry including texture coordinates (uploading it
      first if necessary) but does not set any render device state or
      use any textures.

      This is useful when applying your own G3D::Shader to an existing
      Surface.

      \deprecated
    */
    virtual void sendGeometry(RenderDevice* rd) const = 0;

    ///////////////////////////////////////////////////////////


    /** 
        Sends the geometry for all of the specified surfaces, each with the corresponding coordinateFrame
        bound as the RenderDevice objectToWorld matrix.
        \deprecated
     */
    static void sendGeometry(RenderDevice* rd, const Array<Surface::Ref>& surface3D);

    /** Render geometry only (no shading), and ignore color (but do perform alpha testing).
        Render only back or front faces (two-sided surfaces render no matter what).

        Does not sort or cull based on the view frustum of the camera like other batch rendering routines.

        Used for early-Z and shadow mapping.
        \deprecated
     */    
    static void renderDepthOnly
    (RenderDevice* rd, 
     const Array<Surface::Ref>& surfaceArray, 
     RenderDevice::CullFace cull);
    
    /**
       Renders an array of models with the full G3D illumination model
       (correct transparency, multiple direct lights, multiple shadow
       mapped lights), optimizing ArticulatedModels separately to
       minimize state changes.  
       
       Calls renderTranslucent() for translucent surface rendering.  If you
       need to perform other rendering before translucents, explicitly remove
       non-opaque objects from \a allModels yourself and then call renderTranslucent
       later.  Note that you can use the shadow maps that were computed by sortAndRender.

       \param shadowMaps As many shadow maps as there are
       shadow casting lights must be provided.  Do not render the shadow maps yourself;
       sortAndRender() does that and puts the results back into the array. 
    */
    static void sortAndRender
    (class RenderDevice*                rd, 
     const class GCamera&               camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const Array<ReferenceCountedPointer<ShadowMap> >&  shadowMaps,
     const Array<SuperShader::PassRef>& extraAdditivePasses,
     AlphaMode                          alphaMode = ALPHA_BINARY);

    static void sortAndRender
    (class RenderDevice*                rd, 
     const class GCamera&               camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const Array< ReferenceCountedPointer<ShadowMap> >& shadowMaps);
    
    static void sortAndRender
    (RenderDevice*                      rd, 
     const GCamera&                     camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const ReferenceCountedPointer<ShadowMap>&  shadowMap = NULL);

    /** Render elements of modelArray, handling transmission reasonably.  Special cased
        code for refracting SuperSurface instances.  Called from sortAndRender().

        Assumes:

         - modelArray is sorted from back to front
         - shadowMapArray has the length of lighting->shadowedLightArray and contains
           already computed shadow maps

        Works correctly, but is inefficient for non-translucent surfaces.

        \param alphaMode Mode for resolving partial coverage (which is independent of transmission)
      */
    static void renderTranslucent
    (RenderDevice*                  rd,
     const Array<Surface::Ref>&     modelArray,
     const Lighting::Ref&           lighting,
     const Array<SuperShader::PassRef>& extraAdditivePasses,
     const Array< ReferenceCountedPointer<ShadowMap> >&   shadowMapArray = Array<ShadowMap::Ref>(),
     RefractionQuality              maxRefractionQuality = RefractionQuality::BEST,
     AlphaMode                      alphaMode = ALPHA_BINARY);    

protected:

    /**
       Implementation must obey the current stencil, depth write, color write, and depth test modes.
       Implementation may freely set the blending, and alpha test modes.

       Default implementation renders the triangles returned by getIndices
       and getGeometry. 

       \deprecated
    */
    virtual void defaultRender(RenderDevice* rd) const = 0;
};

/////////////////////////////////////////////////////////////////

typedef ReferenceCountedPointer<class Surface2D> Surface2DRef;

/** Primarily for use in GUI rendering. */
class Surface2D : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Surface2D> Ref;

    /** Assumes that the RenderDevice is configured in in RenderDevice::push2D mode. */
    virtual void render(RenderDevice* rd) const = 0;

    /** Conservative 2D rendering bounds.
     */
    virtual Rect2D bounds() const = 0;

    /**
     2D objects are drawn from back to front, creating the perception of overlap.
     0 = closest to the front, 1 = closest to the back. 
     */
    virtual float depth() const = 0;

    /** Sorts from farthest to nearest. */
    static void sort(Array<Surface2DRef>& array);

    /** Calls sort, RenderDevice::push2D, and then render on all elements */
    static void sortAndRender(RenderDevice* rd, Array<Surface2DRef>& array);
};

} // namespace G3D

#endif // G3D_Surface_h
