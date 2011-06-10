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
#include "G3D/MeshAlg.h"
#include "GLG3D/Texture.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/ShadowMap.h"
#include "G3D/units.h"

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

   May subclasses of Surface need to bind shader and other state in
   order to render.  To amortize the cost of doing so, renderers use
   categorizeByDerivedType<Surface::Ref> to distinguish subclasses and
   then invoke individual rendering methods on arrays of surface
   subclasses at once.

   Most methods take a \a timeOffset argument.  This is the time in seconds
   to offset the result from the time at which the model was posed.  The 
   location of the rendered object is only an approximation when this value
   is non-zero.  For most Surface subclasses, small negative offsets produce fairly
   accurate positioning because the object can be interpolated from the previous pose-time state.
   Positive offsets lead to extrapolation and are often less accurate.
   Note that one could also render at multiple times by posing the original
   models at different times.  However, models do not guarantee that they will
   produce the same number of Surface%s, or Surface%s with the same topology
   each time that they are posed.  The use of timeOffset allows the caller
   to assume that the geometry deforms but has the same topology across an
   interval.

 */
class Surface : public ReferenceCountedObject {
protected:

    Surface() {}

public:

    typedef ReferenceCountedPointer<class Surface> Ref;

    /** \brief How sortAndRender() configures the RenderDevice to process alpha */
    enum AlphaMode {
        /** Alpha > 0.5 is rendered, alpha <= 0.5 is discarded. */
        ALPHA_BINARY,

        /** Convert alpha to coverage values using <code>glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB)</code>. 
           Requires a MSAA framebuffer to be bound.*/
        // See http://www.dhpoware.com/samples/glMultiSampleAntiAliasing.html for an example.
        ALPHA_TO_COVERAGE,
        
        /** Render surfaces with partial coverage from back to front, using Porter and Duff's OVER operator.
            This leaves the depth buffer inconsistent with the color buffer and requires a sort, but often gives 
            the best appearance.
         */
        ALPHA_BLEND
    };

    virtual void getCoordinateFrame(CoordinateFrame& cframe, float timeOffset = 0.0f * units::seconds()) const = 0;

    virtual void getObjectSpaceBoundingBox(AABox& box, float timeOffset = 0.0f) const = 0;

    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, float timeOffset = 0.0f) const = 0;

    /** \brief Clears the arrays and appends indexed triangle list information. 
    
     Most subclasses will ignore \a timeOffset because they only use
     that for rigid-body transformations.  However, it is possible to include skinning
     or keyframe information in a Surface and respond to timeOffset.
    
     Not required to be implemented.*/
    virtual void getObjectSpaceGeometry(Array<int>& index, Array<Point3>& vertex, Array<Vector3>& normal, Array<Vector4>& packedTangent, Array<Point2>& texCoord, float timeOffset = 0.0f) {}

#if 0

    /**
     Forward-render all illumination terms.

     Invoking this with elements of \a surfaceArray that are not of the same most-derived type as \a this will result in an error.

     \param timeOffset All lighting occurs at timeOffset = 0, but object positions should be moved to respect the timeOffset.
     */
    virtual void render(RenderDevice* rd, Array<Surface::Ref>& surface, const Lighting::Ref& lighting, float timeOffset = 0.0f) const;

    /** 
    \brief Render all instances of \a surfaceArray to the currently-bound Framebuffer using the fields and mapping 
    dictated by \a specification.  This is also used for depth-only (z-prepass) rendering.

    Invoking this with elements of \a surfaceArray that are not of the same most-derived type as \a this will result in an error.
    */
    virtual void renderGBuffer(RenderDevice* rd, Array<Surface::Ref>& surfaceArray, const GBufer::Specification& specification, float timeOffset = 0.0f) const = 0;


    /** \brief Rendering a set of surfaces in wireframe, using the current blending mode. 
       This is primarily used for debugging.
       
    Invoking this with elements of \a surfaceArray that are not of the same most-derived type as \a this will result in an error.
       */
    virtual void renderWireframe(RenderDevice* rd, const Array<Surface::Ref>& models, const Color4& color = Color3::black(), float timeOffset = 0.0f) const = 0;

    // TODO: Why is renderShadowMappedLightPass deprecated?  
    //
    // Can we just use renderSuperShaderPass for everything?

#endif

    virtual ~Surface() {}

    virtual std::string name() const = 0;

    /** A hint to the renderer indicating that this surface should
        write to the depth buffer.  Typically overridden to return
        false for surfaces with very low partial coverage (alpha) or
        transmission values, or to resolve artifacts for specific
        scenes.  The default value is ! hasTransmission().*/
    virtual bool depthWriteHint(float distanceToCamera) const {
        (void)distanceToCamera;
        return ! hasTransmission();
    }

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

    /** 
      Divides the inModels into a front-to-back sorted array of opaque
      models and a back-to-front sorted array of potentially
      transparent models.  Any data originally in the output arrays is
      cleared.

      \param wsLookVector Sort axis; usually the -Z axis of the camera.
     */
    static void sortFrontToBack
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector);

    static void sortBackToFront
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector) {
        sortFrontToBack(surfaces, -wsLookVector);
    }

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

    /** Utility function for rendering a set of surfaces in wireframe using the current blending mode. */
    static void renderWireframe(RenderDevice* rd, const Array<Surface::Ref>& models, const Color4& color = Color3::black());

    /** Computes the world-space bounding box of an array of Surface%s.*/
    static void getBoxBounds(const Array<Surface::Ref>& surfaceArray, AABox& bounds);

    /** Computes the world-space bounding sphere of an array of Surface%s.*/
    static void getSphereBounds(const Array<Surface::Ref>& surfaceArray, Sphere& bounds);

    /** Computes the array of models that can be seen by \a camera*/
    static void cull(const class GCamera& camera, const class Rect2D& viewport, const Array<Surface::Ref>& allModels, Array<Surface::Ref>& outModels);

    /**
     Removes elements from \a all and puts them in \a translucent.
     \a translucent is cleared first.
     Always treats hasTransmissive() objects as translucent.
     If \a partialCoverageIsTranslucent is true, also treats hasPartialCoverage as translucent.
     */
    static void extractTranslucent(Array<Surface::Ref>& all, Array<Surface::Ref>& translucent, bool partialCoverageIsTranslucent);

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
