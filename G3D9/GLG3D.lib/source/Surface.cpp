/**
  \file Surface.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2003-11-15
  \edited  2011-06-10
 */ 

#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "GLG3D/ShadowMap.h"
#include "G3D/GCamera.h"
#include "G3D/debugPrintf.h"
#include "G3D/Log.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "G3D/typeutils.h"
#include "GLG3D/Surface.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"
#include "GLG3D/SuperSurface.h"

namespace G3D {

bool ignoreBool;

void Surface::renderIntoGBuffer
   (RenderDevice*               rd,
    Array<Surface::Ref>&        surfaceArray,
    const GBuffer::Ref&         gbuffer,
    const CFrame&               previousCameraFrame) {

    // Sort front-to-back for best early-depth performance
    // (we avoid an early depth pass because we don't know if the depth complexity warrants it)
    sortFrontToBack(surfaceArray, rd->cameraToWorldMatrix().lookVector());

    // Separate by type.  This preserves the sort order and ensures that the closest
    // object will still render first.
    Array< Array<Surface::Ref> > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);

    rd->pushState(gbuffer->framebuffer());
    for (int t = 0; t < derivedTable.size(); ++t) {
        Array<Surface::Ref>& derivedArray = derivedTable[t];
        debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
        derivedArray[0]->renderIntoGBufferHomogeneous(rd, derivedArray, gbuffer, previousCameraFrame);
    }
    rd->popState();
}


void Surface::sendGeometry(RenderDevice* rd, const Array<Surface::Ref>& surface3D) {
    rd->pushState();
    for (int i = 0; i < surface3D.size(); ++i) {
        const Surface::Ref& surface = surface3D[i];
        CFrame coordinateFrame;
        surface->getCoordinateFrame(coordinateFrame, false);
        rd->setObjectToWorldMatrix(coordinateFrame);
        surface->sendGeometry(rd);
    }
    rd->popState();
}


void Surface::getBoxBounds(const Array<Surface::Ref>& models, AABox& bounds, bool previous, bool& anyInfinite, bool onlyShadowCasters) {
    bounds = AABox::empty();

    for (int i = 0; i < models.size(); ++i) {
        const Surface::Ref& surface = models[i];

        if (! onlyShadowCasters || surface->castsShadows()) {
            AABox temp;
            CFrame cframe;
            surface->getCoordinateFrame(cframe, previous);
            surface->getObjectSpaceBoundingBox(temp, previous);

            // Ignore infinite bounding boxes
            if (temp.isFinite()) {
                cframe.toWorldSpace(temp, temp);
                bounds.merge(temp);
            } else {
                anyInfinite = true;
            }
        }
    }
}


void Surface::renderWireframeHomogeneous(RenderDevice* rd, const Array<Surface::Ref>& surface3D, const Color4& color, bool previous) const {
    rd->pushState(); {
        rd->setDepthWrite(false);
        rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setPolygonOffset(-0.5f);

        rd->setColor(color);
        rd->setShader(NULL);
        rd->setLineWidth(0.8f);

        for (int i = 0; i < surface3D.size(); ++i) {
            surface3D[i]->render(rd);
            /*
            CFrame cframe;
            surface3D[i]->getCoordinateFrame(cframe, previous);
            rd->setObjectToWorldMatrix(cframe);
            surface3D[i]->sendGeometry(rd);
            */
        }
    } rd->popState();
}


void Surface::renderWireframe(RenderDevice* rd, const Array<Surface::Ref>& surfaceArray, const Color4& color, bool previous) {
    // Separate by type.  This preserves the sort order and ensures that the closest
    // object will still render first.
    Array< Array<Surface::Ref> > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);

    for (int t = 0; t < derivedTable.size(); ++t) {
        Array<Surface::Ref>& derivedArray = derivedTable[t];
        debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
        derivedArray[0]->renderWireframeHomogeneous(rd, derivedArray, color, previous);
    }
}


void Surface::getSphereBounds(const Array<Surface::Ref>& models, Sphere& bounds, bool previous, bool& anyInfnite, bool onlyShadowCasters) {
    AABox temp;
    getBoxBounds(models, temp, previous, anyInfnite, onlyShadowCasters);
    temp.getBounds(bounds);
}


void Surface::cull
(const GCamera&             camera, 
 const Rect2D&              viewport, 
 const Array<Surface::Ref>& allModels,
 Array<Surface::Ref>&       outModels,
 bool                       previous) {
    debugAssert(&allModels != &outModels);
    outModels.fastClear();

    Array<Plane> clipPlanes;
    camera.getClipPlanes(viewport, clipPlanes);
    for (int i = 0; i < allModels.size(); ++i) {
        Sphere sphere;
        CFrame c;
        Surface::Ref m = allModels[i];
        m->getCoordinateFrame(c, previous);
        m->getObjectSpaceBoundingSphere(sphere, previous);
        sphere = c.toWorldSpace(sphere);

        if (! sphere.culledBy(clipPlanes)) {
            outModels.append(allModels[i]);
        }
    }
}


void Surface::cull
(const GCamera&             camera, 
 const Rect2D&              viewport, 
 Array<Surface::Ref>&       allModels,
 bool                       previous) {
     
    Array<Plane> clipPlanes;
    camera.getClipPlanes(viewport, clipPlanes);
    for (int i = 0; i < allModels.size(); ++i) {
        Sphere sphere;
        CFrame c;
        Surface::Ref m = allModels[i];
        m->getCoordinateFrame(c, previous);
        m->getObjectSpaceBoundingSphere(sphere, previous);
        sphere = c.toWorldSpace(sphere);

        if (sphere.culledBy(clipPlanes)) {
            allModels.fastRemove(i);
            --i;
        }
    }
}

void Surface::renderDepthOnly
(RenderDevice*              rd, 
 const Array<Surface::Ref>& surfaceArray,
 RenderDevice::CullFace     cull) {

    rd->pushState(); {

        rd->setCullFace(cull);
        rd->setDepthWrite(true);
        rd->setColorWrite(false);

        Array< Array<Surface::Ref> > derivedTable;
        categorizeByDerivedType(surfaceArray, derivedTable);

        for (int t = 0; t < derivedTable.size(); ++t) {
            Array<Surface::Ref>& derivedArray = derivedTable[t];
            debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
            // debugPrintf("Invoking on type %s\n", typeid(*derivedArray[0]).raw_name());
            derivedArray[0]->renderDepthOnlyHomogeneous(rd, derivedArray);
        }

    } rd->popState();
}


void Surface::sortAndRender
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<Surface::Ref>&     allModels, 
 const LightingRef&             _lighting, 
 const Array<ShadowMap::Ref>&   shadowMaps,
 const Array<SuperShader::PassRef>& extraAdditivePasses,
 AlphaMode                      alphaMode,
 bool                           updateShadowMaps) {

    const bool previous = false;

    static bool recurse = false;

    alwaysAssertM(! recurse, "Cannot call Surface::sortAndRender recursively");
    recurse = true;

    Lighting::Ref lighting = _lighting->clone();

    int numShadowCastingLights = lighting->numShadowCastingLights();
    
    bool renderShadows =
        (shadowMaps.size() > 0) && 
        (numShadowCastingLights > 0) && 
        shadowMaps[0]->enabled();

    if (renderShadows) {
        // Disable shadows from excess lights
        for (int i = lighting->lightArray.size() - 1; (i >= 0) && (shadowMaps.size() < numShadowCastingLights); --i) {
            if (lighting->lightArray[i].castsShadows) {
                lighting->lightArray[i].castsShadows = false;
                --numShadowCastingLights;
            }
        } 
        if (updateShadowMaps) {
 
            // Find the scene bounds
            AABox shadowCasterBounds;
            Surface::getBoxBounds(allModels, shadowCasterBounds, false, ignoreBool, true);

            Array<Surface::Ref> lightVisible;

            // Generate shadow maps
            int s = 0;
            for (int L = 0; L < lighting->lightArray.size(); ++L) {
                const GLight& light = lighting->lightArray[L];
                if (light.castsShadows) {

                    GCamera lightFrame;
                    Matrix4 lightProjectionMatrix;
                
                    ShadowMap::computeMatrices(light, shadowCasterBounds, lightFrame, lightProjectionMatrix);
                
                    // Cull objects not visible to the light
                    Surface::cull(lightFrame, shadowMaps[s]->rect2DBounds(), allModels, lightVisible, previous);

                    // Cull objects that don't cast shadows
                    for (int i = 0; i < lightVisible.size(); ++i) {
                        if (! lightVisible[i]->castsShadows()) {
                            lightVisible.fastRemove(i);
                            --i;
                        }
                    }

                    Surface::sortFrontToBack(lightVisible, lightFrame.coordinateFrame().lookVector());
                    shadowMaps[s]->updateDepth(rd, lightFrame.coordinateFrame(), lightProjectionMatrix, lightVisible);

                    lightVisible.fastClear();
                    ++s;
                }
            }
        }
    } else {
        // We're not going to be able to draw shadows, so move the shadowed lights into
        // the unshadowed category.
        for (int L = 0; L < lighting->lightArray.size(); ++L) {
            lighting->lightArray[L].castsShadows = false;
        }
        numShadowCastingLights = 0;
    }

    // All objects visible to the camera; gets stripped down to opaque non-super
    static Array<Surface::Ref> visible;

    // Cull objects outside the view frustum
    cull(camera, rd->viewport(), allModels, visible, previous);

    rd->pushState();

    // In the ALPHA_BLEND case we strip out opaque partial coverage surfaces,
    // otherwise they get lumped into the opaque arrays.
    static Array<Surface::Ref>
        super,                        // SuperSurfaces with no translucency
        translucent;                  // Transmission or alpha

    const Vector3 viewVector = camera.coordinateFrame().lookVector();
    Surface::extractTranslucent(visible, translucent, alphaMode == ALPHA_BLEND);
    Surface::sortBackToFront(translucent, viewVector);
    SuperSurface::extract(visible, super);

    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
    switch (alphaMode) {
    case ALPHA_BINARY:
        rd->setAlphaTest(RenderDevice::ALPHA_GEQUAL, 0.5f);
        break;

    case ALPHA_TO_COVERAGE:
        glPushAttrib(GL_MULTISAMPLE_BIT_ARB);
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
        break;

    case ALPHA_BLEND:
        rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.0f);
        break;
    }
    // Get early-out depth test by rendering the closest objects first
    Surface::sortFrontToBack(super,   viewVector);
    Surface::sortFrontToBack(visible, viewVector);

    rd->setProjectionAndCameraMatrix(camera);
    rd->setObjectToWorldMatrix(CoordinateFrame());

    // Opaque unshadowed
    for (int m = 0; m < visible.size(); ++m) {
        visible[m]->renderNonShadowed(rd, lighting);
    }
    SuperSurface::renderNonShadowed(super, rd, lighting);

    // Additively blend the additional passes
    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
    // Opaque shadowed
    int s = 0;
    for (int L = 0; L < lighting->lightArray.size(); ++L) {
        const GLight& light = lighting->lightArray[L];
        if (light.castsShadows) {
            for (int m = 0; m < visible.size(); ++m) {
                visible[m]->renderShadowMappedLightPass(rd, light, shadowMaps[s]);
            }
            rd->pushState();
            SuperSurface::renderShadowMappedLightPass(super, rd, light, shadowMaps[s]);
            rd->popState();
            ++s;
        }
    }

    // Extra additive passes
    if (extraAdditivePasses.size() > 0) {
        rd->pushState();
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
            for (int p = 0; p < extraAdditivePasses.size(); ++p) {
                for (int m = 0; m < visible.size(); ++m) {
                    rd->pushState(); {
                        visible[m]->renderSuperShaderPass(rd, extraAdditivePasses[p]);
                    } rd->popState();
                }
                for (int m = 0; m < super.size(); ++m) {
                    rd->pushState(); {
                        super[m]->renderSuperShaderPass(rd, extraAdditivePasses[p]);
                    } rd->popState();
                }
        }
        rd->popState();
    }

    // Transparent objects must be rendered from back to front after everything else
    renderTranslucent(rd, translucent, lighting, extraAdditivePasses, 
                      shadowMaps, RefractionQuality::BEST, alphaMode);
    super.fastClear();
    translucent.fastClear();
    visible.fastClear();

    if (alphaMode == ALPHA_TO_COVERAGE) {
        glPopAttrib();
    }
    rd->popState();

    recurse = false;
}


void Surface::extractTranslucent
(Array<Surface::Ref>& all, 
 Array<Surface::Ref>& translucent, 
 bool partialCoverageIsTranslucent) {

     translucent.fastClear();
     for (int i = 0; i < all.size(); ++i) {
         if (all[i]->hasTransmission() || (partialCoverageIsTranslucent && all[i]->hasPartialCoverage())) {
             translucent.append(all[i]);
             all.fastRemove(i);
             --i;
         }
     }
}

void Surface::sortAndRender
(class RenderDevice*            rd, 
 const class GCamera&           camera,
 const Array<SurfaceRef>&       allModels, 
 const LightingRef&             _lighting, 
 const Array<ShadowMap::Ref>&   shadowMaps) {
    sortAndRender(rd, camera, allModels, _lighting, shadowMaps, Array<SuperShader::PassRef>());
}


void Surface::sortAndRender
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<Surface::Ref>&     posed3D, 
 const LightingRef&             lighting, 
 const ShadowMap::Ref&          shadowMap) {

    static Array<ShadowMap::Ref> shadowMaps;
    if (shadowMap.notNull()) {
        shadowMaps.append(shadowMap);
    }
    sortAndRender(rd, camera, posed3D, lighting, shadowMaps, Array<SuperShader::PassRef>());
    shadowMaps.fastClear();
}


void Surface2D::sortAndRender(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    if (posed2D.size() > 0) {
        rd->push2D();
            Surface2D::sort(posed2D);
            for (int i = 0; i < posed2D.size(); ++i) {
                posed2D[i]->render(rd);
            }
        rd->pop2D();
    }
}


class ModelSorter {
public:
    float                  sortKey;
    Surface::Ref           model;

    ModelSorter() {}

    ModelSorter(const Surface::Ref& m, const Vector3& axis) : model(m) {
        Sphere s;
        CFrame c;
        m->getCoordinateFrame(c, false);
        m->getObjectSpaceBoundingSphere(s, false);
        sortKey = axis.dot(c.pointToWorldSpace(s.center));
    }

    inline bool operator>(const ModelSorter& s) const {
        return sortKey > s.sortKey;
    }

    inline bool operator<(const ModelSorter& s) const {
        return sortKey < s.sortKey;
    }
};


void Surface::sortFrontToBack
(Array<Surface::Ref>& surface, 
 const Vector3&       wsLook) {

    static Array<ModelSorter> sorter;
    
    for (int m = 0; m < surface.size(); ++m) {
        sorter.append(ModelSorter(surface[m], wsLook));
    }

    sorter.sort(SORT_INCREASING);

    for (int m = 0; m < sorter.size(); ++m) {
        surface[m] = sorter[m].model;
    }

    sorter.fastClear();
}


void Surface::renderNonShadowed(
    RenderDevice* rd,
    const LightingRef& lighting) const {

    rd->pushState(); {
        if (rd->colorWrite()) {
            rd->setAmbientLightColor(Color3::white() * 0.5);
            Array<GLight> ns;
            lighting->getNonShadowCastingLights(ns);
            for (int L = 0; L < iMin(7, ns.size()); ++L) {
                rd->setLight(L, ns[L]);
            }
            rd->enableLighting();
        }
        render(rd);
    } rd->popState();
}


void Surface::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const ShadowMap::Ref& shadowMap) const {

    renderShadowMappedLightPass(rd, light, shadowMap->biasedLightMVP(), shadowMap->depthTexture());
}


void Surface::renderShadowMappedLightPass(
    RenderDevice* rd, 
    const GLight& light,
    const Matrix4& lightMVP,
    const Texture::Ref& shadowMap) const {

    rd->pushState();
    {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->configureShadowMap(1, lightMVP, shadowMap);
        rd->setLight(0, light);
        rd->enableLighting();
        rd->setAmbientLightColor(Color3::black());
        render(rd);
    }
    rd->popState();
}

   
void Surface::render(RenderDevice* rd) const {
    defaultRender(rd);
}


void Surface::renderTranslucent
(RenderDevice*                  rd,
 const Array<Surface::Ref>&     modelArray,
 const Lighting::Ref&           lighting,
 const Array<SuperShader::PassRef>& extraAdditivePasses,
 const Array<ShadowMap::Ref>&   shadowMapArray,
 RefractionQuality              maxRefractionQuality,
 AlphaMode                      alphaMode) {

    const bool previous = false;

    rd->pushState();

    debugAssertGLOk();
    
    Framebuffer::Ref fbo = rd->framebuffer();
    const ImageFormat* screenFormat = rd->colorFormat();
    if (fbo.isNull()) {
        rd->setReadBuffer(RenderDevice::READ_BACK);
    } else {
        Framebuffer::Attachment::Ref screen = fbo->get(Framebuffer::COLOR0);
        debugAssertM(screen.notNull(), "No color attachment on framebuffer");
        rd->setReadBuffer(RenderDevice::READ_FRONT);
    }
    debugAssertGLOk();

    static bool supportsRefract = Shader::supportsPixelShaders() && GLCaps::supports_GL_ARB_texture_non_power_of_two();

    static Texture::Ref refractBackground;
    static Shader::Ref refractShader;

    bool didReadback = false;

    const CFrame& cameraFrame = rd->cameraToWorldMatrix();

    bool oldDepthWrite = rd->depthWrite();

    // Switching between fixed function and SuperSurfaces mode is expensive.
    // This flag lets us avoid the switch when rendering multiple SuperSurfaces
    // sequentially that have no transmission.
    bool inSuperSurfaceAlphaMode = false;

    for (int m = 0; m < modelArray.size(); ++m) {
        Surface::Ref model = modelArray[m];

        CFrame cframe;
        Sphere sphere;
        model->getCoordinateFrame(cframe, previous);
        model->getObjectSpaceBoundingSphere(sphere, previous);
        const float distanceToCamera = (sphere.center + cframe.translation - cameraFrame.translation).dot(cameraFrame.lookVector());

        rd->setDepthWrite(oldDepthWrite && model->depthWriteHint(distanceToCamera));

        const SuperSurface::Ref& gmodel = model.downcast<SuperSurface>();

        // Render refraction (without modulating by transmissive)
        if (gmodel.notNull() && model->hasTransmission() && supportsRefract) {
            Material::Ref material = gmodel->gpuGeom()->material;
            const float eta = material->bsdf()->etaTransmit();

            if ((eta > 1.001f) && 
                (material->refractionHint() >= RefractionQuality::DYNAMIC_FLAT) &&
                (material->refractionHint() <= RefractionQuality::DYNAMIC_FLAT_MULTILAYER) &&
                (maxRefractionQuality >= RefractionQuality::DYNAMIC_FLAT)) {

                if (! didReadback || (material->refractionHint() == RefractionQuality::DYNAMIC_FLAT_MULTILAYER)) {
                    if (refractBackground.isNull()) {
                        refractBackground = Texture::createEmpty("Background", rd->width(), rd->height(), screenFormat, 
                                                                 Texture::DIM_2D_NPOT, Texture::Settings::video());
                    }

                    rd->copyTextureFromScreen(refractBackground, rd->viewport(), screenFormat);
                    didReadback = true;
                }

                if (refractShader.isNull()) {
                    refractShader = Shader::fromFiles(System::findDataFile("SS_Refract.vrt"), 
                                                      System::findDataFile("SS_Refract.pix"));
                }
                
                // Perform refraction.  The above test ensures that the
                // back-side of a surface (e.g., glass-to-air interface)
                // does not perform refraction
                rd->pushState();
                {
                    rd->setDepthWrite(false);
                    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
                    const Sphere& bounds3D = cframe.toWorldSpace(sphere);
                    
                    // Estimate of distance from object to background to
                    // be constant (we could read back depth buffer, but
                    // that won't produce frame coherence)
                    
                    const float z0 = max(8.0f - (eta - 1.0f) * 5.0f, bounds3D.radius);
                    const float backZ = cameraFrame.pointToObjectSpace(bounds3D.center).z - z0;
                    refractShader->args.set("backZ", backZ);
                    
                    // Assume rays are leaving air
                    refractShader->args.set("etaRatio", 1.0f / eta);
                    
                    // Find out how big the back plane is in meters
                    float backPlaneZ = min(-0.5f, backZ);
                    GCamera cam2 = rd->projectionAndCameraMatrix();
                    cam2.setFarPlaneZ(backPlaneZ);
                    Vector3 ur, ul, ll, lr;
                    cam2.getFarViewportCorners(rd->viewport(), ur, ul, ll, lr);
                    Vector2 backSizeMeters((ur - ul).length(), (ur - lr).length());
                    refractShader->args.set("backSizeMeters", backSizeMeters);
                    refractShader->args.set("background", refractBackground);
                    refractShader->args.set("backgroundInvertY", rd->framebuffer().notNull());
                    refractShader->args.set("lambertianMap", Texture::whiteIfNull(material->bsdf()->lambertian().texture()));
                    refractShader->args.set("lambertianConstant", material->bsdf()->lambertian().constant());
                    rd->setShader(refractShader);
                    rd->setObjectToWorldMatrix(cframe);
                    gmodel->sendGeometry(rd);
                }
                rd->popState();
            }
        }

        if (model->hasTransmission()) {
            if (model->hasPartialCoverage()) {
                if (alphaMode == ALPHA_BLEND) {
                    // Transmission, and alpha, and alpha blending.
                    // Modulate down the source contribution.
                    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE);
                } else {
                    // Transmission, binary or alpha-to-coverage alpha. Just add.
                    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
                }
            } else {
                // Transmission, no alpha.  Just add.
                rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
            }
        } else if (model->hasPartialCoverage()) {
            if (alphaMode == ALPHA_BLEND) {
                // No transmission, blended alpha
                rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            } else {
                // No transmission, binary or alpha-to-coverage alpha: treat as opaque
                rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
            }
        } else {
            // Actually opaque!  Why did we get here in the translucent renderer?
            debugAssert(false);
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
        }

        static Array<Surface::Ref> oneSurface(NULL);
        if (gmodel.notNull() && ! model->hasTransmission()) {
            if (! inSuperSurfaceAlphaMode) {
                // Enter SuperSurface mode
                rd->pushState();
                inSuperSurfaceAlphaMode = true;
            } 

            // Call the non-preserveState version of renderNonShadowed
            oneSurface[0] = model;
            SuperSurface::renderNonShadowed(oneSurface, rd, lighting, false);

        } else {

            if (inSuperSurfaceAlphaMode) {
                // Exit SS mode
                rd->popState();
                inSuperSurfaceAlphaMode = false;
            }
            // Add lights, or black
            model->renderNonShadowed(rd, lighting);
        }

        // Add shadowed lights
        if ((alphaMode == ALPHA_BLEND) && model->hasPartialCoverage()) {
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE);     
        } else {
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        }
        int s = 0;
        for (int L = 0; L < lighting->lightArray.size(); ++L) {
            const GLight& light = lighting->lightArray[L];
            if (light.castsShadows) {
                if (inSuperSurfaceAlphaMode) {
                    // If we've made it to this branch, oneSurface is already set
                    SuperSurface::renderShadowMappedLightPass(oneSurface, rd, light, shadowMapArray[s], false);
                } else {
                    model->renderShadowMappedLightPass(rd, light, shadowMapArray[s]);
                }
                ++s;
            }
        }

        // Add extra light passes
        if (extraAdditivePasses.size() > 0) {
            if (! inSuperSurfaceAlphaMode) {
                rd->pushState();
            }
            for (int p = 0; p < extraAdditivePasses.size(); ++p) {
                model->renderSuperShaderPass(rd, extraAdditivePasses[p]);
            }
            if (! inSuperSurfaceAlphaMode) {
                rd->popState();
            }
        }
    }

    if (inSuperSurfaceAlphaMode) {
        // Exit SS mode
        rd->popState();
        inSuperSurfaceAlphaMode = false;
    }
    rd->popState();
}
////////////////////////////////////////////////////////////////////////////////////////

static bool depthGreaterThan(const Surface2DRef& a, const Surface2DRef& b) {
    return a->depth() > b->depth();
}

void Surface2D::sort(Array<Surface2DRef>& array) {
    array.sort(depthGreaterThan);
}



void Surface::getTris(const Array<Surface::Ref>& surfaceArray, CPUVertexArray& cpuVertexArray, Array<Tri>& triArray){
    Array< Array<Surface::Ref> > derivedTable;
    categorizeByDerivedType(surfaceArray, derivedTable);
    for (int t = 0; t < derivedTable.size(); ++t) {
        Array<Surface::Ref>& derivedArray = derivedTable[t];
        derivedArray[0]->getTrisHomogeneous(derivedArray, cpuVertexArray, triArray);
    }

}


}
