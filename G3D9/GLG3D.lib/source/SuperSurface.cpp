/**
  \file GLG3D.lib/source/SuperSurface.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2004-11-20
  \edited  2011-06-11

  Copyright 2001-2011, Morgan McGuire
 */
#include "G3D/Log.h"
#include "G3D/fileutils.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Lighting.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperShader.h"
#include "GLG3D/Draw.h" // TODO remove

namespace G3D {

class GBufferShaderCache {
protected:
    
    typedef Table<std::string, Shader::Ref> Cache;
    Cache cache;

public:

    Shader::Ref get(const GBuffer::Ref& gbuffer, const Material::Ref& material) {
        static const std::string version = "#version 120\n#extension GL_EXT_gpu_shader4 : require\n";

        const std::string& prefixMacros = version + "\n// GBuffer macros:\n" + gbuffer->macros() +
            "\n// Material macros:\n\n" + material->macros() + "///////////////////\n\n";

        bool created = false;
        Shader::Ref& shader = cache.getCreate(prefixMacros, created);

        if (created) {
            // Create the shader

            static const std::string commonVertexSource    = readWholeFile(System::findDataFile("SS_GBuffer.vrt"));
            static const std::string commonGeometrySource  = readWholeFile(System::findDataFile("SS_GBuffer.geo"));
            static const std::string commonPixelSource     = readWholeFile(System::findDataFile("SS_GBuffer.pix"));

            const std::string& vertexSource = prefixMacros + commonVertexSource;

            // The geometry shader is only needed if face normals are required
            const std::string& geometrySource =
                (gbuffer->hasFaceNormals()) ?
                (prefixMacros + commonGeometrySource) :
                std::string("");

            const std::string& pixelSource = prefixMacros + commonPixelSource;

            // Compile
            shader = Shader::fromStrings(vertexSource, geometrySource, pixelSource);
            shader->setPreserveState(false);
        }

        return shader;
    }
};

static GBufferShaderCache gbufferShaderCache;

void SuperSurface::renderIntoGBufferHomogeneous
(RenderDevice*                rd, 
 Array<Surface::Ref>&         surfaceArray,
 const GBuffer::Ref&          gbuffer,
 float                        timeOffset,
 float                        velocityStartOffset) const {
    rd->pushState(); {
        rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
        const RenderDevice::CullFace oldCullFace = rd->cullFace();

        for (int s = 0; s < surfaceArray.size(); ++s) {
            const SuperSurface::Ref& surface = surfaceArray[s].downcast<SuperSurface>();
            debugAssertM(surface != NULL, "Non SuperSurface element of surfaceArray in SuperSurface::renderIntoGBufferHomogeneous");

            const GPUGeom::Ref& gpuGeom = surface->gpuGeom();
            const Material::Ref& material = gpuGeom->material;

            // This is frequently the same shader as on the previous iteration, and RenderDevice will
            // optimize out the state change. 
            const Shader::Ref& shader = gbufferShaderCache.get(gbuffer, material);
            rd->setShader(shader);

            CFrame cframe;
            surface->getCoordinateFrame(cframe, timeOffset);
            rd->setObjectToWorldMatrix(cframe);

            if (gpuGeom->twoSided) {
                rd->setCullFace(RenderDevice::CULL_NONE);
            }

            // Bind material arguments
            material->configure(shader->args);

            // TODO: pass alpha threshold

            surface->sendGeometry(rd);

            if (gpuGeom->twoSided) {
                rd->setCullFace(oldCullFace);
            }
        }
    } rd->popState();
}


/** For fixed function, we detect
    reflection as having no glossy map but a packed specular
   exponent of 1 (=infinity).*/   
static bool mirrorReflectiveFF(const SuperBSDF::Ref& bsdf) { 
    return 
        (bsdf->specular().factors() == Component4::CONSTANT) &&
        (SuperBSDF::packedSpecularMirror() == bsdf->specular().constant().a);
}

/** For fixed function, we detect glossy
    reflection as having a packed specular exponent less 
    than 1.*/   
static bool glossyReflectiveFF(const SuperBSDF::Ref& bsdf) {
     return (bsdf->specular().constant().a != SuperBSDF::packedSpecularMirror()) && 
            (bsdf->specular().constant().a != SuperBSDF::packedSpecularNone());
}


bool SuperSurface::depthWriteHint(float distanceToCamera) const {
    const float d = m_gpuGeom->material->depthWriteHintDistance();
    if (isNaN(d)) {
        return ! hasTransmission();
    } else {
        return distanceToCamera < d;
    }
}


void SuperSurface::sortFrontToBack(Array<SuperSurface::Ref>& a, const Vector3& v) {
    Array<Surface::Ref> s;
    s.resize(a.size());
    for (int i = 0; i < s.size(); ++i) {
        s[i] = a[i];
    }
    Surface::sortFrontToBack(s, v);
    for (int i = 0; i < s.size(); ++i) {
        a[i] = s[i].downcast<SuperSurface>();
    }
}

SuperSurface::Ref SuperSurface::create
(const std::string&       name,
 const CFrame&            frame, 
 const GPUGeom::Ref&      gpuGeom,
 const CPUGeom&           cpuGeom,
 const ReferenceCountedPointer<ReferenceCountedObject>& source) {
    debugAssert(gpuGeom.notNull());


    // Cannot check if the gpuGeom is valid because it might not be filled out yet
    return new SuperSurface(name, frame, gpuGeom, cpuGeom, source);
}


static void setAdditive(RenderDevice* rd, bool& additive);

int SuperSurface::debugNumSendGeometryCalls = 0;

// Static to this file, not the class
static SuperSurface::GraphicsProfile graphicsProfile = SuperSurface::UNKNOWN;

void SuperSurface::setProfile(GraphicsProfile p) {
    graphicsProfile = p;
}


SuperSurface::GraphicsProfile SuperSurface::profile() {

    if (graphicsProfile == UNKNOWN) {
        if (GLCaps::supports_GL_ARB_shader_objects()) {
            graphicsProfile = PS20;

            
            if (System::findDataFile("SS_NonShadowedPass.vrt") == "") {
                graphicsProfile = UNKNOWN;
                logPrintf("\n\nWARNING: SuperSurface could not enter PS20 mode because"
                          "NonShadowedPass.vrt was not found.\n\n");
            }
        }

        
        if (graphicsProfile == UNKNOWN) {
            graphicsProfile = FIXED_FUNCTION;
        }
    }

    return graphicsProfile;
}


const char* toString(SuperSurface::GraphicsProfile p) {
    switch (p) {
    case SuperSurface::UNKNOWN:
        return "Unknown";

    case SuperSurface::FIXED_FUNCTION:
        return "Fixed Function";

    case SuperSurface::PS20:
        return "PS 2.0";

    default:
        return "Error!";
    }
}


void SuperSurface::renderNonShadowed(
    const Array<Surface::Ref>&      posedArray, 
    RenderDevice*                   rd, 
    const LightingRef&              lighting,
    bool                            preserveState) {

    if (posedArray.size() == 0) {
        return;
    }

    if (! rd->depthWrite() && ! rd->colorWrite()) {
        // Nothing to draw!
        return;
    }

    RenderDevice::BlendFunc srcBlend;
    RenderDevice::BlendFunc dstBlend;
    RenderDevice::BlendEq   blendEq;
    rd->getBlendFunc(srcBlend, dstBlend, blendEq);

    if (preserveState) {
        rd->pushState();
    }
    {
        bool originalDepthWrite = rd->depthWrite();

        // Lighting will be turned on and off by subroutines
        rd->disableLighting();

        const bool ps20 = SuperSurface::profile() == SuperSurface::PS20;

        for (int p = 0; p < posedArray.size(); ++p) {
            const SuperSurface::Ref& posed = posedArray[p].downcast<SuperSurface>();

            if (! rd->colorWrite()) {
                // No need for fancy shading, just send geometry
                posed->sendGeometry2(rd);
                continue;
            }

            const Material::Ref& material = posed->m_gpuGeom->material;
            const SuperBSDF::Ref& bsdf = material->bsdf();
            (void)material;
            (void)bsdf;
            debugAssertM(bsdf->transmissive().isBlack(), 
                "Transparent object passed through the batch version of "
                "SuperSurface::renderNonShadowed, which is intended exclusively for opaque objects.");

            if (posed->m_gpuGeom->twoSided) {
                if (! ps20) {
                    rd->enableTwoSidedLighting();
                    rd->setCullFace(RenderDevice::CULL_NONE);
                } else {
                    // Even if back face culling is reversed, for two-sided objects 
                    // we always draw the front.
                    rd->setCullFace(RenderDevice::CULL_BACK);
                }
            }
            bool wroteDepth = posed->renderNonShadowedOpaqueTerms(rd, lighting, false);

            if (posed->m_gpuGeom->twoSided && ps20) {
                // gl_FrontFacing doesn't work on most cards inside
                // the shader, so we have to draw two-sided objects
                // twice
                rd->setCullFace(RenderDevice::CULL_FRONT);
                
                wroteDepth = posed->renderNonShadowedOpaqueTerms(rd, lighting, false) || wroteDepth;
            }

            rd->setDepthWrite(originalDepthWrite);
            if (! wroteDepth && originalDepthWrite) {
                // We failed to write to the depth buffer, so
                // do so now.
                rd->disableLighting();
                rd->setColor(Color3::black());
                rd->setShader(NULL);
                if (posed->m_gpuGeom->twoSided) {
                    rd->setCullFace(RenderDevice::CULL_NONE);
                }
                posed->sendGeometry2(rd);
                rd->enableLighting();
            }

            if (posed->m_gpuGeom->twoSided) {
                rd->disableTwoSidedLighting();
                rd->setCullFace(RenderDevice::CULL_BACK);
            }

            // Alpha blend will be changed by subroutines so we restore it for each object
            rd->setBlendFunc(srcBlend, dstBlend, blendEq);
            rd->setDepthWrite(originalDepthWrite);
        }
    }
    if (preserveState) {
        rd->popState();
    }
}


/** Swaps the definition of "Front" and "back" if the original culling face was backwards. */
static void setCullFace(RenderDevice* rd, RenderDevice::CullFace newCull, RenderDevice::CullFace original) {
    if (original == RenderDevice::CULL_FRONT) {
        switch (newCull) {
        case RenderDevice::CULL_FRONT:
            rd->setCullFace(RenderDevice::CULL_BACK);
            break;

        case RenderDevice::CULL_BACK:
            rd->setCullFace(RenderDevice::CULL_FRONT);
            break;
            
        default:
            rd->setCullFace(newCull);
        }
    } else {
        rd->setCullFace(newCull);
    }
}


void SuperSurface::renderShadowMappedLightPass
(const Array<Surface::Ref>&     posedArray, 
 RenderDevice*                  rd, 
 const GLight&                  light, 
 const ShadowMap::Ref&          shadowMap,
 bool                           preserveState) {
    
    if (posedArray.size() == 0) {
        return;
    }

    const RenderDevice::CullFace oldCullFace = rd->cullFace();
    RenderDevice::BlendFunc oldSrcBlendFunc = RenderDevice::BLEND_ONE, oldDstBlendFunc = RenderDevice::BLEND_ONE;
    RenderDevice::BlendEq oldBlendEq = RenderDevice::BLENDEQ_MIN;
    if (preserveState) {
        rd->pushState();
    } else {
        rd->getBlendFunc(oldSrcBlendFunc, oldDstBlendFunc, oldBlendEq);
    }
    {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        setCullFace(rd, RenderDevice::CULL_BACK, oldCullFace);

        for (int i = 0; i < posedArray.size(); ++i) {
            const SuperSurface::Ref&    posed    = posedArray[i].downcast<SuperSurface>();
            const Material::Ref&        material = posed->m_gpuGeom->material;
            const SuperBSDF::Ref&       bsdf     = material->bsdf();

            if (bsdf->lambertian().isBlack() && bsdf->specular().isBlack()) {
                // Nothing to draw for this object
                continue;
            }

            // This switch could go outside the loop, however doing so would lead to code repetition
            // without any real increase in performance.

            switch (profile()) {
            case FIXED_FUNCTION:
                if (posed->m_gpuGeom->twoSided) {
                    rd->enableTwoSidedLighting();
                    setCullFace(rd, RenderDevice::CULL_NONE, oldCullFace);
                }

                posed->renderFFShadowMappedLightPass(rd, light, shadowMap);

                if (posed->m_gpuGeom->twoSided) {
                    rd->disableTwoSidedLighting();
                    setCullFace(rd, RenderDevice::CULL_BACK, oldCullFace);
                }
                break;

            case PS20:
                // Even if back face culling is reversed, for two-sided objects 
                // we always draw the front first.
                setCullFace(rd, RenderDevice::CULL_BACK, oldCullFace);

                posed->renderPS20ShadowMappedLightPass(rd, light, shadowMap, oldCullFace);

                if (posed->m_gpuGeom->twoSided) {
                    // The GLSL built-in gl_FrontFacing does not work on most cards, so we have to draw 
                    // two-sided objects twice since there is no way to distinguish them in the shader.
                    setCullFace(rd, RenderDevice::CULL_FRONT, oldCullFace);
                    posed->renderPS20ShadowMappedLightPass(rd, light, shadowMap, oldCullFace);
                    setCullFace(rd, RenderDevice::CULL_BACK, oldCullFace);
                }
                break;

            default:
                debugAssertM(false, "Fell through switch");
            }
        }
    }
    if (preserveState) {
        rd->popState();
    } else {
        rd->setCullFace(oldCullFace);
        rd->setBlendFunc(oldSrcBlendFunc, oldDstBlendFunc, oldBlendEq);
    }
 }


void SuperSurface::extract(
    Array<Surface::Ref>&   all, 
    Array<Surface::Ref>&   super) {
    
    for (int i = 0; i < all.size(); ++i) {
        ReferenceCountedPointer<SuperSurface> m = 
            dynamic_cast<SuperSurface*>(all[i].pointer());

        if (m.notNull()) {
            // This is a most-derived subclass and is opaque

            super.append(m);
            all.fastRemove(i);
            // Iterate over again
            --i;
        }
    }
}


/////////////////////////////////////////////////////////////////


void SuperSurface::render(RenderDevice* rd) const {

    // Infer the lighting from the fixed function

    // Avoid allocating memory for each render
    static LightingRef lighting = Lighting::create();

    rd->getFixedFunctionLighting(lighting);

    renderNonShadowed(rd, lighting);
}


bool SuperSurface::renderSuperShaderPass
(RenderDevice*                rd, 
 const SuperShader::PassRef&  pass,
 RenderDevice::CullFace       originalCullFace) const {

    if (m_gpuGeom->twoSided) {
        // We're going to render the front and back faces separately.
        setCullFace(rd, RenderDevice::CULL_FRONT, originalCullFace);
        rd->setShader(pass->getConfiguredShader(*m_gpuGeom->material, rd->cullFace()));
        sendGeometry2(rd);
    }

    setCullFace(rd, RenderDevice::CULL_BACK, originalCullFace);
    rd->setShader(pass->getConfiguredShader(*m_gpuGeom->material, rd->cullFace()));
    sendGeometry2(rd);

    return false;
}


/** 
 Switches to additive rendering, if not already in that mode.
 */
static void setAdditive(RenderDevice* rd, bool& additive) {
    if (! additive) {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->setDepthWrite(false);
        additive = true;
    }
}


bool SuperSurface::renderNonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting,
    bool preserveState) const {

    bool renderedOnce = false;

    switch (profile()) {
    case FIXED_FUNCTION:
        renderedOnce = renderFFNonShadowedOpaqueTerms(rd, lighting);
        break;

    case PS20:
        if (preserveState) {
            rd->pushState();
        }
        renderedOnce = renderPS20NonShadowedOpaqueTerms(rd, lighting);
        if (preserveState) {
            rd->popState();
        }
        break;

    default:
        debugAssertM(false, "Fell through switch");
    }        

    return renderedOnce;
}


bool SuperSurface::renderPS20NonShadowedOpaqueTerms(
    RenderDevice*         rd,
    const Lighting::Ref&  lighting) const {
    const float timeOffset = 0.0f;

    const Material::Ref&  material = m_gpuGeom->material;
    const SuperBSDF::Ref& bsdf     = material->bsdf();

    RenderDevice::BlendFunc srcBlend;
    RenderDevice::BlendFunc dstBlend;
    RenderDevice::BlendEq   blendEq;
    rd->getBlendFunc(srcBlend, dstBlend, blendEq);

    // Note that partial coverage surfaces must always be rendered opaquely, even if black, because
    // the alpha value may affect the image.
    if (! ((bsdf->hasReflection() || (srcBlend != RenderDevice::BLEND_ONE))
        || (m_gpuGeom->material->emissive().notBlack() && ! lighting->emissiveScale.isZero()))) {
        // Nothing to draw
        return false;
    }

    LightingRef reducedLighting = lighting->clone();
    reducedLighting->removeShadowCastingLights();

    int numLights = reducedLighting->lightArray.size();

    if (numLights <= SuperShader::NonShadowedPass::LIGHTS_PER_PASS) {
        
        SuperShader::NonShadowedPass::instance()->setLighting(reducedLighting);
        rd->setShader(SuperShader::NonShadowedPass::instance()->getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));

        sendGeometry2(rd);

    } else {
        LightingRef originalReducedLighting = reducedLighting->clone();

        // SuperShader only supports SuperShader::NonShadowedPass::LIGHTS_PER_PASS lights, so we have to make multiple passes
        Array<GLight> lights = reducedLighting->lightArray;

        Sphere myBounds;
        CFrame cframe;
        getObjectSpaceBoundingSphere(myBounds, timeOffset);
        getCoordinateFrame(cframe, timeOffset);
        myBounds = cframe.toWorldSpace(myBounds);
        
        // Remove lights that cannot affect this object
        for (int L = 0; L < lights.size(); ++L) {
            Sphere s = lights[L].effectSphere();
            if (! s.intersects(myBounds)) {
                // This light does not affect this object
                lights.fastRemove(L);
                --L;
            }
        }
        numLights = lights.size();

        // Number of lights to use
        int x = iMin(SuperShader::NonShadowedPass::LIGHTS_PER_PASS, lights.size());

        // Copy the lights into the reduced lighting structure
        reducedLighting->lightArray.resize(x);
        SuperShader::NonShadowedPass::instance()->setLighting(reducedLighting);
        rd->setShader(SuperShader::NonShadowedPass::instance()->getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));
        sendGeometry2(rd);

        if (numLights > SuperShader::NonShadowedPass::LIGHTS_PER_PASS) {
            // Add extra lighting terms
            rd->pushState();
            rd->setBlendFunc(RenderDevice::BLEND_CURRENT, RenderDevice::BLEND_ONE);
            rd->setDepthWrite(false);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            for (int L = SuperShader::NonShadowedPass::LIGHTS_PER_PASS; 
                 L < numLights; 
                 L += SuperShader::ExtraLightPass::LIGHTS_PER_PASS) {

                SuperShader::ExtraLightPass::instance()->setLighting(originalReducedLighting->lightArray, L);
                rd->setShader(SuperShader::ExtraLightPass::instance()->
                              getConfiguredShader(*(m_gpuGeom->material), rd->cullFace()));
                sendGeometry2(rd);
            }
            rd->popState();
        }
    }

    return true;
}


bool SuperSurface::renderFFNonShadowedOpaqueTerms(
    RenderDevice*                   rd,
    const LightingRef&              lighting) const {

    debugAssertGLOk();

    bool renderedOnce = false;

    const Material::Ref& material = m_gpuGeom->material;
    const SuperBSDF::Ref&     bsdf = material->bsdf();


    // Emissive
    if (! material->emissive().isBlack()) {
        rd->setColor(material->emissive().constant());
        rd->setTexture(0, material->emissive().texture());
        sendGeometry2(rd);
        setAdditive(rd, renderedOnce);
    }

    // Add reflective.  

    if (! mirrorReflectiveFF(bsdf) &&
        lighting.notNull() &&
        (lighting->environmentMapConstant != 0.0f)) {

        rd->pushState();

            // Reflections are specular and not affected by surface texture, only
            // the reflection coefficient
            rd->setColor(bsdf->specular().constant().rgb() * lighting->environmentMapConstant);

            // TODO: Use diffuse alpha map here somehow?
            rd->setTexture(0, NULL);

            // Configure reflection map
            if (lighting->environmentMapTexture.isNull()) {
                rd->setTexture(1, NULL);
            } else if (GLCaps::supports_GL_ARB_texture_cube_map() &&
                       (lighting->environmentMapTexture->dimension() == Texture::DIM_CUBE_MAP)) {
                rd->configureReflectionMap(1, lighting->environmentMapTexture);
            } else {
                // Use the top texture as a sphere map
                glActiveTextureARB(GL_TEXTURE0_ARB + 1);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);

                rd->setTexture(1, lighting->environmentMapTexture);
            }

            sendGeometry2(rd);
            setAdditive(rd, renderedOnce);

            // Disable reflection map
            rd->setTexture(1, NULL);
        rd->popState();
    }
    debugAssertGLOk();

    // Add ambient + lights
    if (bsdf->lambertian().factors() != Component4::BLACK || 
        bsdf->specular().factors() != Component4::BLACK) {
        rd->enableLighting();
        rd->setTexture(0, bsdf->lambertian().texture());
        rd->setColor(bsdf->lambertian().constant());

        // Fixed function does not receive specular texture maps, only constants.
        rd->setSpecularCoefficient(bsdf->specular().constant().rgb());
        rd->setShininess(SuperBSDF::unpackSpecularExponent(bsdf->specular().constant().a));
        debugAssertGLOk();

        // Ambient
        if (lighting.notNull()) {
            // Lights
            for (int L = 0; L < iMin(7, lighting->lightArray.size()); ++L) {
                rd->setLight(L + 1, lighting->lightArray[L]);
            }
        }
        debugAssertGLOk();

        if (renderedOnce) {
            // Make sure we add this pass to the previous already-rendered terms
            rd->setBlendFunc(RenderDevice::BLEND_CURRENT, RenderDevice::BLEND_ONE);
        }

        sendGeometry2(rd);
        renderedOnce = true;
        rd->disableLighting();
    }

    return renderedOnce;
}


void SuperSurface::renderNonShadowed
(RenderDevice*        rd,
 const LightingRef&   lighting) const {

    // The transparent rendering path is not optimized to amortize state changes because 
    // it is only called by the single-object version of this function.  Only
    // opaque objects are batched together.
    const SuperBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    if (hasTransmission()) {
        rd->pushState(); {
            // Transparent
            bool oldDepthWrite = rd->depthWrite();

            // Render backfaces first, and then front faces.  Each will be culled exactly
            // once, so we aren't going to overdraw.
            int passes = m_gpuGeom->twoSided ? 2 : 1;

            if (m_gpuGeom->twoSided) {
                // We're going to render the front and back faces separately.
                rd->setCullFace(RenderDevice::CULL_BACK);
            }

            for (int i = 0; i < passes; ++i) {
                // Modulate background by transparent color
                // TODO: Fresnel for shader-driven pipeline
                rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_SRC_COLOR);
                rd->setTexture(0, bsdf->transmissive().texture());
                rd->setColor(bsdf->transmissive().constant());
                sendGeometry2(rd);

                // Draw normal terms on top
                bool alreadyAdditive = false;
                setAdditive(rd, alreadyAdditive);
                renderNonShadowedOpaqueTerms(rd, lighting, false);
                      
                // restore depth write
                rd->setDepthWrite(oldDepthWrite);
                rd->setCullFace(RenderDevice::CULL_BACK);
            }
        } rd->popState();
    } else {

        // This is the unoptimized, single-object version of renderNonShadowed.
        // It just calls the optimized version with a single-element array.

        static Array<Surface::Ref> posedArray;
        posedArray.resize(1);
        posedArray[0] = Ref(const_cast<SuperSurface*>(this));
        renderNonShadowed(posedArray, rd, lighting);
        posedArray.fastClear();
    }
}


void SuperSurface::renderShadowedLightPass(
    RenderDevice*       rd, 
    const GLight&       light) const {

    (void)rd;
    (void)light;
    // TODO
    debugAssertM(false, "Unimplemented");
}


void SuperSurface::renderShadowMappedLightPass(
    RenderDevice*       rd, 
    const GLight&       light, 
    const Matrix4&      lightMVP, 
    const Texture::Ref&   shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.
    debugAssertM(false, "Deprecated: use the method that takes a ShadowMap");
}


void SuperSurface::renderShadowMappedLightPass(
    RenderDevice*         rd, 
    const GLight&         light, 
    const ShadowMap::Ref& shadowMap) const {

    // This is the unoptimized, single-object version of renderShadowMappedLightPass.
    // It just calls the optimized version with a single-element array.

    static Array<Surface::Ref> posedArray;

    posedArray.resize(1);
    posedArray[0] = Ref(const_cast<SuperSurface*>(this));
    renderShadowMappedLightPass(posedArray, rd, light, shadowMap);
    posedArray.fastClear();
}


void SuperSurface::renderPS20ShadowMappedLightPass
(RenderDevice*          rd,
 const GLight&          light, 
 const ShadowMap::Ref&  shadowMap,
 RenderDevice::CullFace originalCullFace) const {

    SuperShader::ShadowedPass::instance()->setLight(light, shadowMap);
    renderSuperShaderPass(rd, SuperShader::ShadowedPass::instance(), originalCullFace);
}


void SuperSurface::renderFFShadowMappedLightPass(
    RenderDevice*       rd,
    const GLight&       light, 
    const ShadowMap::Ref& shadowMap) const {

    rd->configureShadowMap(1, shadowMap);

    rd->setObjectToWorldMatrix(m_frame);

    const SuperBSDF::Ref& bsdf = m_gpuGeom->material->bsdf();

    rd->setTexture(0, bsdf->lambertian().texture());
    rd->setColor(bsdf->lambertian().constant());

    // We disable specular highlights because they will not be modulated
    // by the shadow map.  We then make a separate pass to render specular
    // highlights.
    rd->setSpecularCoefficient(Color3::zero());

    rd->enableLighting();
    rd->setAmbientLightColor(Color3::black());

    rd->setLight(0, light);

    sendGeometry2(rd);

    if (glossyReflectiveFF(bsdf)) {
        // Make a separate pass for specular. 
        static bool separateSpecular = GLCaps::supports_GL_EXT_separate_specular_color();

        if (separateSpecular) {
            // We disable the OpenGL separate
            // specular behavior so that the texture will modulate the specular
            // pass, and then put the specularity coefficient in the texture.
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SINGLE_COLOR_EXT);
        }

        rd->setColor(Color3::white()); // TODO: when I put the specular coefficient here, it doesn't modulate.  What's wrong?
        rd->setTexture(0, bsdf->specular().texture());
        rd->setSpecularCoefficient(bsdf->specular().constant().rgb());

        // Turn off the diffuse portion of this light
        GLight light2 = light;
        rd->setLight(0, light);
        rd->setShininess(SuperBSDF::unpackSpecularExponent(bsdf->specular().constant().a));

        static const Color4 zero(0, 0, 0, 1);
        //glLightfv(gi, GL_DIFFUSE,           reinterpret_cast<const float*>(&zero));
        //sendGeometry2(rd);

        if (separateSpecular) {
            // Restore normal behavior
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, 
                          GL_SEPARATE_SPECULAR_COLOR_EXT);
        }

        // TODO: use this separate specular pass code in all fixed function 
        // cases where there is a specularity map.
    }
}


void SuperSurface::sendGeometry2(
    RenderDevice*           rd) const {

    debugAssertGLOk();

    // Make a copy, not a reference; we're about to mutate the
    // underlying state
    const CoordinateFrame o2w = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(m_frame);

    rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
    sendGeometry(rd);

    rd->setObjectToWorldMatrix(o2w);

}


void SuperSurface::sendGeometry(
    RenderDevice*           rd) const {
    debugAssertGLOk();
    ++debugNumSendGeometryCalls;

    debugAssertGLOk();
    rd->beginIndexedPrimitives();
    {
        rd->setVertexArray(m_gpuGeom->vertex);
        debugAssertGLOk();

        debugAssert(m_gpuGeom->normal.valid());
        rd->setNormalArray(m_gpuGeom->normal);
        debugAssertGLOk();

        if (m_gpuGeom->texCoord0.valid() && (m_gpuGeom->texCoord0.size() > 0)){
            rd->setTexCoordArray(0, m_gpuGeom->texCoord0);
            debugAssertGLOk();
        }

        // In programmable pipeline mode, load the tangents into tex coord 1
        if (m_gpuGeom->packedTangent.valid() && (profile() == PS20) && (m_gpuGeom->packedTangent.size() > 0)) {
            rd->setTexCoordArray(1, m_gpuGeom->packedTangent);
            debugAssertGLOk();
        }

        debugAssertGLOk();
        rd->sendIndices(m_gpuGeom->primitive, m_gpuGeom->index);
    }
    rd->endIndexedPrimitives();
}


std::string SuperSurface::name() const {
    return m_name;
}


bool SuperSurface::hasTransmission() const {
    return ! m_gpuGeom->material->bsdf()->transmissive().isBlack();
}


bool SuperSurface::hasPartialCoverage() const {
    return m_gpuGeom->material->bsdf()->lambertian().nonUnitAlpha();
}


void SuperSurface::getCoordinateFrame(CoordinateFrame& c, float timeOffset) const {
    c = m_frame;
}


void SuperSurface::getObjectSpaceBoundingSphere(Sphere& s, float timeOffset) const {
    s = m_gpuGeom->sphereBounds;
}


void SuperSurface::getObjectSpaceBoundingBox(AABox& b, float timeOffset) const {
    b = m_gpuGeom->boxBounds;
}


void SuperSurface::CPUGeom::copyVertexDataToGPU
(VertexRange&               vertex, 
 VertexRange&               normal, 
 VertexRange&               packedTangentVAR, 
 VertexRange&               texCoord0VAR, 
 VertexBuffer::UsageHint    hint) {

    int vtxSize = sizeof(Vector3) * geometry->vertexArray.size();
    int texSize = sizeof(Vector2) * texCoord0->size();
    int tanSize = sizeof(Vector4) * packedTangent->size();

    if ((vertex.maxSize() >= vtxSize) &&
        (normal.maxSize() >= vtxSize) &&
        ((tanSize == 0) || (packedTangentVAR.maxSize() >= tanSize)) &&
        ((texSize == 0) || (texCoord0VAR.maxSize() >= texSize))) {
        VertexRange::updateInterleaved
           (geometry->vertexArray,  vertex,
            geometry->normalArray,  normal,
            *packedTangent,         packedTangentVAR,
            *texCoord0,             texCoord0VAR);

    } else {

        // Maximum round-up size of varArea.
        int roundOff = 16;

        // Allocate new VARs
        VertexBuffer::Ref varArea = VertexBuffer::create(vtxSize * 2 + texSize + tanSize + roundOff, hint);
        VertexRange::createInterleaved
            (geometry->vertexArray, vertex,
             geometry->normalArray, normal,
             *packedTangent,        packedTangentVAR,
             *texCoord0,            texCoord0VAR,
             varArea);       
    }
}

}
