/**
 \file Texture.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2001-02-28
 \edited  2012-04-21
*/
#include "G3D/Log.h"
#include "G3D/Any.h"
#include "G3D/Matrix3.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "G3D/ThreadSet.h"
#include "G3D/ImageFormat.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/Texture.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/BumpMap.h"

#ifdef verify
#undef verify
#endif

namespace G3D {

static bool isMipMapFormat(Texture::InterpolateMode i) {
    switch (i) {
    case Texture::TRILINEAR_MIPMAP:
    case Texture::BILINEAR_MIPMAP:
    case Texture::NEAREST_MIPMAP:
        return true;

    case Texture::BILINEAR_NO_MIPMAP:
    case Texture::NEAREST_NO_MIPMAP:
        return false;
    }

    alwaysAssertM(false, "Illegal interpolate mode");
    return false;
}

/** Used by various Texture methods when a framebuffer is needed */
static const Framebuffer::Ref& workingFramebuffer() {
    static Framebuffer::Ref fbo = Framebuffer::create("Texture FBO");
    return fbo;
}


Color4 Texture::readTexel(int x, int y, RenderDevice* rd) const {
    debugAssertGLOk();
    const Framebuffer::Ref& fbo = workingFramebuffer();

    if (rd == NULL) {
        rd = RenderDevice::lastRenderDeviceCreated;
    }

    Color4 c;

    // Read back 1 pixel
    if (format()->depthBits == 0) {

        fbo->set(Framebuffer::COLOR0, Texture::Ref(this));
        rd->pushState(fbo);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &c);
        rd->popState();
    } else {
        // This is a depth texture
        fbo->set(Framebuffer::DEPTH, Texture::Ref(this));
        rd->pushState(fbo);
        glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &c.r);
        rd->popState();
        c.g = c.b = c.a = c.r;
    }

    fbo->clear();
    return c;
}


const Texture::CubeMapInfo& Texture::cubeMapInfo(CubeMapConvention convention) {
    static CubeMapInfo cubeMapInfo[CubeMapConvention::COUNT];
    static bool initialized = false;
    if (! initialized) {
        initialized = true;
        cubeMapInfo[CubeMapConvention::QUAKE].name = "Quake";
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].suffix = "bk";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].suffix = "ft";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].suffix = "up";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].suffix = "dn";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].suffix = "rt";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].suffix = "lf";


        cubeMapInfo[CubeMapConvention::UNREAL].name = "Unreal";
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].suffix = "east";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].suffix = "west";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].suffix = "up";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].suffix = "down";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].suffix = "south";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].suffix = "north";


        cubeMapInfo[CubeMapConvention::G3D].name = "G3D";
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].suffix = "+x";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].suffix = "-x";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].suffix = "+y";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].suffix = "-y";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].suffix = "+z";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].suffix = "-z";


        cubeMapInfo[CubeMapConvention::DIRECTX].name = "DirectX";
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].suffix = "PX";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].suffix = "NX";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].suffix = "PY";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].suffix = "NY";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].suffix = "PZ";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].suffix = "NZ";
    }

    return cubeMapInfo[convention];
}


Any Texture::Preprocess::toAny() const {
    Any a(Any::TABLE, "Texture::Preprocess");
    a["modulate"] = modulate;
    a["gammaAdjust"] = gammaAdjust;
    a["scaleFactor"] = scaleFactor;
    a["computeMinMaxMean"] = computeMinMaxMean;
    a["computeNormalMap"] = computeNormalMap;
    a["bumpMapPreprocess"] = bumpMapPreprocess;
    return a;
}


bool Texture::Preprocess::operator==(const Preprocess& other) const {
    return 
        (modulate == other.modulate) &&
        (gammaAdjust == other.gammaAdjust) &&
        (scaleFactor == other.scaleFactor) &&
        (computeMinMaxMean == other.computeMinMaxMean) &&
        (computeNormalMap == other.computeNormalMap) &&
        (bumpMapPreprocess == other.bumpMapPreprocess);
}


Texture::Preprocess::Preprocess(const Any& any) {
    *this = Preprocess::defaults();
    any.verifyNameBeginsWith("Texture::Preprocess");
    if (any.type() == Any::TABLE) {
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const std::string& key = it->key;
            if (key == "modulate") {
                modulate = Color4(it->value);
            } else if (key == "gammaAdjust") {
                gammaAdjust = it->value;
            } else if (key == "scaleFactor") {
                scaleFactor = it->value;
            } else if (key == "computeMinMaxMean") {
                computeMinMaxMean = it->value;
            } else if (key == "computeNormalMap") {
                computeNormalMap = it->value;
            } else if (key == "bumpMapPreprocess") {
                bumpMapPreprocess = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
    } else {
        const std::string& n = any.name();
        if (n == "Texture::Preprocess::defaults") {
            any.verifySize(0);
            // Done!
        } else if (n == "Texture::Preprocess::gamma") {
            any.verifySize(1);
            *this = Texture::Preprocess::gamma(any[0]);
        } else if (n == "Texture::preprocess::none") {
            any.verifySize(0);
            *this = Texture::Preprocess::none();
        } else if (n == "Texture::Preprocess::quake") {
            any.verifySize(0);
            *this = Texture::Preprocess::quake();
        } else if (n == "Texture::Preprocess::normalMap") {
            any.verifySize(0);
            *this = Texture::Preprocess::normalMap();
        } else {
            any.verify(false, "Unrecognized name for Texture::Preprocess constructor or factory method.");
        }
    }
}

const Texture::Preprocess& Texture::Preprocess::defaults() {
    static const Texture::Preprocess p;
    return p;
}


Texture::Preprocess Texture::Preprocess::gamma(float g) {
    Texture::Preprocess p;
    p.gammaAdjust = g;
    return p;
}


const Texture::Preprocess& Texture::Preprocess::none() {
    static Texture::Preprocess p;
    p.computeMinMaxMean = false;
    return p;
}


const Texture::Preprocess& Texture::Preprocess::quake() {
    static Texture::Preprocess p;
    p.modulate = Color4(2,2,2,1);
    p.gammaAdjust = 1.6f;
    return p;
}

const Texture::Preprocess& Texture::Preprocess::normalMap() {
    static bool initialized = false;
    static Texture::Preprocess p;
    if (! initialized) {
        p.computeNormalMap = true;
        initialized = true;
    }

    return p;
}


Texture::Dimension Texture::defaultDimension() {
    static const Texture::Dimension dim = 
        GLCaps::supports_GL_ARB_texture_non_power_of_two() ? DIM_2D_NPOT : DIM_2D;

    return dim;
}


CubeMapConvention Texture::determineCubeConvention(const std::string& filename) {
    std::string filenameBase, filenameExt;
    Texture::splitFilenameAtWildCard(filename, filenameBase, filenameExt);
    if (FileSystem::exists(filenameBase + "east" + filenameExt)) {
        return CubeMapConvention::UNREAL;
    } else if (FileSystem::exists(filenameBase + "lf" + filenameExt)) {
        return CubeMapConvention::QUAKE;
    } else if (FileSystem::exists(filenameBase + "+x" + filenameExt)) {
        return CubeMapConvention::G3D;
    } else if (FileSystem::exists(filenameBase + "PX" + filenameExt) || FileSystem::exists(filenameBase + "px" + filenameExt)) {
        return CubeMapConvention::DIRECTX;
    }
    throw std::string("File not found");
    return CubeMapConvention::G3D;
}

static void generateCubeMapFilenames(const std::string& src, std::string realFilename[6], Texture::CubeMapInfo& info) {
    std::string filenameBase, filenameExt;
    Texture::splitFilenameAtWildCard(src, filenameBase, filenameExt);

    CubeMapConvention convention = Texture::determineCubeConvention(src);

    info = Texture::cubeMapInfo(convention);
    for (int f = 0; f < 6; ++f) {
        realFilename[f] = filenameBase + info.face[f].suffix + filenameExt;
    }
}


int64 Texture::m_sizeOfAllTexturesInMemory = 0;

/**
 Returns true if the system supports automatic MIP-map generation.
 */
static bool hasAutoMipMap();

/**
 Pushes all OpenGL texture state.
 */
// TODO: this is slow; push only absolutely necessary state
// or back up state that will be corrupted.
static void glStatePush();

/**
 Pops all OpenGL texture state.
 */
static void glStatePop() {
    glPopClientAttrib();
    glPopAttrib();
}  


Texture::Ref Texture::createColor(const Color3unorm8& c) {
    Texture::Specification s;
    s.filename = "<white>";
    s.preprocess.modulate = Color4unorm8(c, unorm8::one());
    s.settings.interpolateMode = NEAREST_NO_MIPMAP;
    s.settings.wrapMode = WrapMode::TILE;
    s.desiredFormat = ImageFormat::RGB8();
    return Texture::create(s);
}


Texture::Ref Texture::createColor(const Color4unorm8& c) {
    Texture::Specification s;
    s.filename = "<white>";
    s.preprocess.modulate = c;   
    s.settings.interpolateMode = NEAREST_NO_MIPMAP;
    s.settings.wrapMode = WrapMode::TILE;
    s.desiredFormat = ImageFormat::RGBA8();
    return Texture::create(s);
}


Texture::Ref Texture::white() {
    static WeakReferenceCountedPointer<Texture> cache;

    Texture::Ref t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0xFF, imageBuffer->size());
        Texture::Settings settings;
        settings.wrapMode = WrapMode::TILE;
        t = Texture::fromImageBuffer("<white>", imageBuffer, ImageFormat::RGB8(), Texture::DIM_2D, settings);
        // Store in cache
        cache = t;
    }

    return t;
}

TextureRef Texture::opaqueBlackCube() {
    static WeakReferenceCountedPointer<Texture> cache;

    TextureRef t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0x00, imageBuffer->size());
        Array< Array<const void*> > bytes;
        Array<const void*>& cubeFace = bytes.next();
        for (int i = 0; i < 6; ++i)  {
            // todo (Image upgrade): investigate pixel1, pixel3 and pixel3 accessors from GImage
            cubeFace.append(static_cast<Color3unorm8*>(imageBuffer->buffer()));
        }
        t = Texture::fromMemory("Opaque black cube", bytes, imageBuffer->format(), imageBuffer->width(), imageBuffer->height(), 1, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, Texture::Settings::cubeMap());

        // Store in cache
        cache = t;
    }

    return t;
}


TextureRef Texture::whiteCube() {
    static WeakReferenceCountedPointer<Texture> cache;

    TextureRef t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0xFF, imageBuffer->size());
        Array< Array<const void*> > bytes;
        Array<const void*>& cubeFace = bytes.next();
        for (int i = 0; i < 6; ++i)  {
            // todo (Image upgrade): investigate pixel1, pixel3 and pixel3 accessors from GImage
            cubeFace.append(static_cast<Color3unorm8*>(imageBuffer->buffer()));
        }
        t = Texture::fromMemory("White cube", bytes, imageBuffer->format(), imageBuffer->width(), imageBuffer->height(), 1, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, Texture::Settings::cubeMap());

        // Store in cache
        cache = t;
    }

    return t;
}

TextureRef Texture::zero() {
    static WeakReferenceCountedPointer<Texture> cache;
    
    TextureRef t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty                                                                                      
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(8, 8, ImageFormat::RGBA8());
        System::memset(imageBuffer->buffer(), 0x00, imageBuffer->size());
        t = Texture::fromImageBuffer("Zero", imageBuffer);
        
        cache = t;
    }
    
    return t;
}

TextureRef Texture::opaqueBlack() {
    static WeakReferenceCountedPointer<Texture> cache;
    
    TextureRef t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty                                                                                      
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(8, 8, ImageFormat::RGBA8());
        for (int i = 0; i < imageBuffer->width() * imageBuffer->height(); ++i) {
            // todo (Image upgrade): investigate pixel1, pixel3 and pixel3 accessors from GImage
            Color4unorm8* pixels = static_cast<Color4unorm8*>(imageBuffer->buffer());
            pixels[i] = Color4unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero(), unorm8::one());
        }
        t = Texture::fromImageBuffer("Opaque Black", imageBuffer);
        
        cache = t;
    }
    
    return t;
}


TextureRef Texture::opaqueGray() {
    static WeakReferenceCountedPointer<Texture> cache;
    
    TextureRef t = cache.createStrongPtr();
    if (t.isNull()) {
        // Cache is empty                                                                                      
        ImageBuffer::Ref imageBuffer = ImageBuffer::create(8, 8, ImageFormat::RGBA8());
        const Color4unorm8 c(Color4(0.5f, 0.5f, 0.5f, 1.0f));
        for (int i = 0; i < imageBuffer->width() * imageBuffer->height(); ++i) {
            // todo (Image upgrade): investigate pixel1, pixel3 and pixel3 accessors from GImage
            Color4unorm8* pixels = static_cast<Color4unorm8*>(imageBuffer->buffer());
            pixels[i] = c;
        }
        t = Texture::fromImageBuffer("Gray", imageBuffer);
        
        cache = t;
    }
    
    return t;
}

void Texture::generateMipMaps(){
    glPushAttrib(GL_TEXTURE_BIT); {
        glBindTexture(openGLTextureTarget(), openGLID());
        glGenerateMipmap(openGLTextureTarget());
    } glPopAttrib();
}


/**
 Scales the intensity up or down of an entire image and gamma corrects.
 */
static void modulateImage(ImageFormat::Code fmt, void* byte, int n, const Color4& brighten, float gamma);

static GLenum dimensionToTarget(Texture::Dimension d);

static void createTexture(
    GLenum          target,
    const uint8*    rawBytes,
    GLenum          bytesActualFormat,
    GLenum          bytesFormat,
    int             m_width,
    int             m_height,
    int             depth,
    GLenum          ImageFormat,
    int             bytesPerPixel,
    int             mipLevel,
    bool            compressed,
    bool            useNPOT,
    float           rescaleFactor,
    GLenum          dataType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval);



static void createMipMapTexture(    
    GLenum          target,
    const uint8*    _bytes,
    int             bytesFormat,
    int             bytesBaseFormat,
    int             m_width,
    int             m_height,
    GLenum          ImageFormat,
    int             bytesFormatBytesPerPixel,
    float           rescaleFactor,
    GLenum          bytesType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval);


/////////////////////////////////////////////////////////////////////////////

Texture::Texture(
    const std::string&      name,
    GLuint                  textureID,
    Dimension               dimension,
    const ImageFormat*      format,
    bool                    opaque,
    const Settings&         settings) :
    m_textureID(textureID),
    m_settings(settings),
    m_name(name),
    m_dimension(dimension),
    m_opaque(opaque),
    m_format(format),
    m_min(Color4::nan()),
    m_max(Color4::nan()),
    m_mean(Color4::nan()) {

    debugAssert(m_format);
    debugAssertGLOk();

    glStatePush();
    {
        // TODO: Explicitly pass these in instead of reading them

        GLenum target = dimensionToTarget(m_dimension);
        glBindTexture(target, m_textureID);

        // For cube map, we can't read "cube map" but must choose a face
        GLenum readbackTarget = target;
        if ((m_dimension == DIM_CUBE_MAP) || (m_dimension == DIM_CUBE_MAP_NPOT)) {
            readbackTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
        }

        glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_WIDTH, &m_width);
        glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_HEIGHT, &m_height);

        if (readbackTarget == GL_TEXTURE_3D) {
            glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_DEPTH, &m_depth);
        }

        m_depth = 1;
        
        debugAssertGLOk();
        setTexParameters(target, m_settings);
        debugAssertGLOk();
    }
    glStatePop();
    debugAssertGLOk();

    m_sizeOfAllTexturesInMemory += sizeInMemory();

    
}


///// Texture ImageBuffer interface start

Texture::Texture(const std::string&         name,
                 const MipsPerCubeFace&     mipsPerCubeFace,
                 Dimension                  dimension,
                 InterpolateMode            interpolation,
                 WrapMode                   wrapping,
                 const ImageFormat*         desiredFormat,
                 const Settings&            settings)
    : m_textureID(0)
    , m_settings(settings)
    , m_name(name)
    , m_dimension(dimension)
    , m_opaque(desiredFormat->opaque)
    , m_format(desiredFormat)
    , m_width(0)
    , m_height(0)
    , m_depth(0)
    , m_min(Color4::nan())
    , m_max(Color4::nan())
    , m_mean(Color4::nan()) {

    // Verify that enough ImageBuffer's were passed in to create a texture
    debugAssert(mipsPerCubeFace.length() > 0);
    debugAssert(mipsPerCubeFace[0].length() > 0);

    if (mipsPerCubeFace.length() == 0 || mipsPerCubeFace[0].length() == 0) {
        debugAssertM(false, "Cannot create Texture without source images");
        return;
    }

    // Validate settings are valid before pre-processing and uploading
    if (! validateSettings()) {
        // validateSettings should throw any asserts necessary
        return;
    }

    // Generate texture id and configure texture settings
    configureTexture(mipsPerCubeFace);

    // Preprocess texture and possibly return new buffers
    // TODO: Should we allow editing in-place?
    const MipsPerCubeFace& preprocessedImages = preprocessImages(mipsPerCubeFace);

    uploadImages(preprocessedImages);

    m_sizeOfAllTexturesInMemory += sizeInMemory();

}

bool Texture::validateSettings() {
    return true;
}

void Texture::configureTexture(const MipsPerCubeFace& mipsPerCubeFace) {
    // Get new texture from OpenGL
    m_textureID = newGLTextureID();
    debugAssertGLOk();

    const ImageBuffer::Ref& fullImage = mipsPerCubeFace[0][0];

    // Get image dimensions
    m_width = fullImage->width();
    m_height = fullImage->height();
    m_depth = fullImage->depth();

    glStatePush();
    {
        // Behind texture to target for configuration
        GLenum target = dimensionToTarget(m_dimension);
        glBindTexture(target, m_textureID);
        debugAssertGLOk();

        // Configure target texture settings
        setTexParameters(target, m_settings);
        debugAssertGLOk();
    }
    glStatePop();
    debugAssertGLOk();
}

Texture::MipsPerCubeFace Texture::preprocessImages(const MipsPerCubeFace& mipsPerCubeFace) {
    return mipsPerCubeFace;
}

void Texture::uploadImages(const MipsPerCubeFace& mipsPerCubeFace) {
    glBindTexture(openGLTextureTarget(), m_textureID);
    debugAssertGLOk();

    for (int mipIndex = 0; mipIndex < mipsPerCubeFace[0].length(); ++mipIndex) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexImage2D(openGLTextureTarget(), mipIndex, m_format->openGLFormat, m_width, m_height, 0, mipsPerCubeFace[0][mipIndex]->format()->openGLBaseFormat, mipsPerCubeFace[0][mipIndex]->format()->openGLDataFormat, mipsPerCubeFace[0][mipIndex]->buffer());
        debugAssertGLOk();
    }
}


///// Texture ImageBuffer interface end


Texture::Ref Texture::fromMemory(
    const std::string&              name,
    const void*                     bytes,
    const class ImageFormat*        bytesFormat,
    int                             m_width,
    int                             m_height,
    int                             depth,
    const class ImageFormat*        desiredFormat,
    Dimension                       dimension,
    const Settings&                 settings,
    const Preprocess&               preprocess) {

    Array< Array<const void*> > data;
    data.resize(1);
    data[0].append(bytes);

    return fromMemory(name, 
                      data,
                      bytesFormat, 
                      m_width, 
                      m_height, 
                      depth, 
                      desiredFormat,
                      dimension,
                      settings,
                      preprocess);
}


void Texture::setAutoMipMap(bool b) {
    m_settings.autoMipMap = b;

    // Update the OpenGL state
    GLenum target = dimensionToTarget(m_dimension);

    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(target, m_textureID);

    if (hasAutoMipMap()) {
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, b ? GL_TRUE : GL_FALSE);
    }

    // Restore the old texture
    glPopAttrib();
}


Texture::Ref Texture::fromGLTexture(
    const std::string&      name,
    GLuint                  textureID,
    const ImageFormat*      imageFormat,
    Dimension               dimension,
    const Settings&            settings) {

    debugAssert(imageFormat);

    return new Texture(
        name, 
        textureID, 
        dimension,
        imageFormat, 
        imageFormat->opaque, 
        settings);
}


static void transform(Image::Ref& image, const Texture::CubeMapInfo::Face& info) {
    // Apply transformations
    if (info.flipX) {
        image->flipHorizontal();
    }
    if (info.flipY) {
        image->flipVertical();
    }
    if (info.rotations > 0) {
        image->rotateCW(toRadians(90.0), info.rotations);
    }
}

Texture::Ref Texture::create(const Specification& s) {
    return Texture::fromFile(s.filename, s.desiredFormat, s.dimension, s.settings, s.preprocess);
}

class ImageLoaderThread : public GThread {
private:
    std::string         m_filename;
    Image::Ref&          m_image;
public:
    
    ImageLoaderThread(const std::string& filename, Image::Ref& im) : 
        GThread(filename), m_filename(filename), m_image(im) {}

    void threadMain() {
        m_image = Image::fromFile(m_filename);
    }
};

Texture::Ref Texture::fromFile(
    const std::string               filename[6],
    const class ImageFormat*        desiredFormat,
    Dimension                       dimension,
    const Settings&                 settings,
    const Preprocess&               preprocess) {
    
    std::string realFilename[6];

    //    const ImageFormat* format = ImageFormat::RGB8();
    //bool opaque = true;

    Array< Array< const void* > > byteMipMapFaces;

    const int numFaces = 
        ((dimension == DIM_CUBE_MAP) || (dimension == DIM_CUBE_MAP_NPOT)) ?
        6 : 1;

    // Single mip-map level
    byteMipMapFaces.resize(1);
    
    Array<const void*>& array = byteMipMapFaces[0];
    array.resize(numFaces);

    debugAssertM(filename[1] == "",
        "Can't specify more than one filename");

    realFilename[0] = filename[0];
    CubeMapInfo info;

    // Ensure that this is not "<white>" before splitting names
    if ((numFaces == 6) && ! beginsWith(filename[0], "<")) {
        // Parse the filename into a base name and extension
        generateCubeMapFilenames(filename[0], realFilename, info);
    }

    // The six cube map faces, or the one texture and 5 dummys.
    Image::Ref image[6];

    if ((numFaces == 1) && (dimension == DIM_2D) || (dimension == DIM_2D_NPOT) || (dimension == DIM_3D_NPOT) || (dimension == DIM_3D)) {
        if (toLower(realFilename[0]) == "<white>") {
            ImageBuffer::Ref buffer = ImageBuffer::create(1, 1, ImageFormat::RGBA8());
            image[0] = Image::fromImageBuffer(buffer);
            image[0]->set(Point2int32(0, 0), Color4unorm8::one());
        } else {
            image[0] = Image::fromFile(realFilename[0]);

            alwaysAssertM(image[0]->width() > 0, 
                G3D::format("Image not found: \"%s\" and GImage failed to throw an exception", realFilename[0].c_str()));
        }
    } else {
        // Load each cube face on a different thread to overlap compute and I/O
        ThreadSet threadSet;

        for (int f = 0; f < numFaces; ++f) {
            if ((toLower(realFilename[f]) == "<white>") || realFilename[f].empty()) {
            ImageBuffer::Ref buffer = ImageBuffer::create(1, 1, ImageFormat::RGBA8());
            image[f] = Image::fromImageBuffer(buffer);
            image[f]->set(Point2int32(0, 0), Color4unorm8::one());
            } else {
                threadSet.insert(new ImageLoaderThread(realFilename[f], image[f]));
            }
        }
        threadSet.start(GThread::USE_CURRENT_THREAD);
        threadSet.waitForCompletion();
    }

    ImageBuffer::Ref buffers[6];
    for (int f = 0; f < numFaces; ++f) {
        //debugPrintf("Loading %s\n", realFilename[f].c_str());
        alwaysAssertM(image[f]->width() > 0, "Image not found");
        alwaysAssertM(image[f]->height() > 0, "Image not found");

        if (numFaces > 1) {
            transform(image[f], info.face[f]);
        }

        buffers[f] = image[f]->toImageBuffer();
        array[f] = buffers[f]->buffer();
    }

    Texture::Ref t =
        fromMemory(filename[0], 
                   byteMipMapFaces, 
                   buffers[0]->format(),
                   buffers[0]->width(), 
                   buffers[0]->height(), 
                   1,
                   desiredFormat, 
                   dimension,
                   settings,
                   preprocess);
    
    return t;
}


Texture::Ref Texture::fromFile
   (const std::string&      filename,
    const ImageFormat*      desiredFormat,
    Dimension               dimension,
    const Settings&         settings,
    const Preprocess&       preprocess) {

    std::string f[6];
    f[0] = filename;
    f[1] = "";
    f[2] = "";
    f[3] = "";
    f[4] = "";
    f[5] = "";

    return fromFile(f, desiredFormat, dimension, settings, preprocess);
}


Texture::Ref Texture::fromTwoFiles(
    const std::string&      filename,
    const std::string&      alphaFilename,
    const ImageFormat*      desiredFormat,
    Dimension               dimension,
    const Settings&            settings,
    const Preprocess&        preprocess) {

    debugAssert(desiredFormat);

    // The six cube map faces, or the one texture and 5 dummys.
    Array< Array<const void*> > mip;
    mip.resize(1);
    Array<const void*>& array = mip[0];

    const int numFaces = 
        ((dimension == DIM_CUBE_MAP) || (dimension == DIM_CUBE_MAP_NPOT)) ? 
        6 : 1;

    array.resize(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        array[i] = NULL;
    }

    // Parse the filename into a base name and extension
    std::string filenameArray[6];
    std::string alphaFilenameArray[6];
    filenameArray[0] = filename;
    alphaFilenameArray[0] = alphaFilename;

    // Test for both DIM_CUBE_MAP and DIM_CUBE_MAP_NPOT
    CubeMapInfo info, alphaInfo;
    if (numFaces == 6) {
        generateCubeMapFilenames(filename, filenameArray, info);
        generateCubeMapFilenames(alphaFilename, alphaFilenameArray, alphaInfo);
    }
    
    Image::Ref color[6];
    Image::Ref alpha[6];
    ImageBuffer::Ref buffers[6];
    Texture::Ref t;

    try {
        for (int f = 0; f < numFaces; ++f) {
            // Compose the two images to a single RGBA
            color[f] = Image::fromFile(filenameArray[f]);
            alpha[f] = Image::fromFile(alphaFilenameArray[f]);
            uint8* data = NULL;


            if (numFaces > 1) {
                transform(color[f], info.face[f]);
                transform(alpha[f], alphaInfo.face[f]);
            }

            if (color[f]->format() == ImageFormat::RGBA8()) {
                // Write the data inline
                for (int h = 0; h < color[f]->height(); ++h) {
                    for (int w = 0; w < color[f]->width(); ++w) {
                        // todo (Image upgrade): double check this re-created logic
                        Color4 origColor;
                        color[f]->get(Point2int32(w, h), origColor);

                        Color4 origAlpha;
                        alpha[f]->get(Point2int32(w, h), origAlpha);

                        color[f]->set(Point2int32(w, h), Color4(origColor.rgb(), origAlpha.a));
                    }
                }

                buffers[f] = color[f]->toImageBuffer();
                data = static_cast<uint8*>(buffers[f]->buffer());
            } else {
                alwaysAssertM(color[f]->format() == ImageFormat::RGB8(), "Cannot load monochrome cube maps");
                buffers[f] = ImageBuffer::create(color[f]->width(), color[f]->height(), ImageFormat::RGBA8());
                Image::Ref tempImage = Image::fromImageBuffer(buffers[f]);
                // Write the data inline
                for (int h = 0; h < color[f]->height(); ++h) {
                    for (int w = 0; w < color[f]->width(); ++w) {
                        // todo (Image upgrade): double check this re-created logic
                        Color4 origColor;
                        color[f]->get(Point2int32(w, h), origColor);

                        Color4 origAlpha;
                        alpha[f]->get(Point2int32(w, h), origAlpha);

                        tempImage->set(Point2int32(w, h), Color4(origColor.rgb(), origAlpha.a));
                    }
                }

                buffers[f] = tempImage->toImageBuffer();
                data = static_cast<uint8*>(buffers[f]->buffer());
            }

            array[f] = data;
        }

        t = fromMemory(
                filename, 
                mip, 
                ImageFormat::RGBA8(),
                buffers[0]->width(), 
                buffers[0]->height(), 
                1, 
                desiredFormat, 
                dimension,
                settings,
                preprocess);

    } catch (const Image::Error& e) {
        Log::common()->printf("\n**************************\n\n"
            "Loading \"%s\" failed. %s\n", e.filename.c_str(),
            e.reason.c_str());
    }

    return t;
}




Texture::Ref Texture::fromMemory(
    const std::string&                  name,
    const Array< Array<const void*> >&  _bytes,
    const ImageFormat*                  bytesFormat,
    int                                 width,
    int                                 height,
    int                                 depth,
    const ImageFormat*                  desiredFormat,
    Dimension                           dimension,
    const Settings&                     settings,
    const Preprocess&                   preprocess) {

    // For use computing normal maps
    ImageBuffer::Ref normal;

    typedef Array< Array<const void*> > MipArray;

    float scaleFactor = preprocess.scaleFactor;
    
    // Indirection needed in case we have to reallocate our own
    // data for preprocessing.
    MipArray* bytesPtr = const_cast<MipArray*>(&_bytes);

    if (dimension == DIM_3D) {
        debugAssertM((settings.interpolateMode == BILINEAR_NO_MIPMAP) ||
                     (settings.interpolateMode == NEAREST_NO_MIPMAP), 
                     "DIM_3D textures do not support mipmaps");
        debugAssertM(_bytes.size() == 1,                    
                     "DIM_3D textures do not support mipmaps");
    } else if (dimension != DIM_3D_NPOT) {
        debugAssertM(depth == 1, "Depth must be 1 for all textures that are not DIM_3D or DIM_3D_NPOT");
    }

    if ((preprocess.modulate != Color4::one()) || (preprocess.gammaAdjust != 1.0f)) {
        debugAssert((bytesFormat->code == ImageFormat::CODE_RGB8) ||
                    (bytesFormat->code == ImageFormat::CODE_RGBA8) ||
                    (bytesFormat->code == ImageFormat::CODE_L8));

        // Allow brightening to fail silently in release mode
        if (( bytesFormat->code == ImageFormat::CODE_L8) ||
            ( bytesFormat->code == ImageFormat::CODE_RGB8) ||
            ( bytesFormat->code == ImageFormat::CODE_RGBA8)) {

            bytesPtr = new MipArray();
            bytesPtr->resize(_bytes.size());

            for (int m = 0; m < bytesPtr->size(); ++m) {
                Array<const void*>& face = (*bytesPtr)[m]; 
                face.resize(_bytes[m].size());
            
                for (int f = 0; f < face.size(); ++f) {

                    int numBytes = iCeil(width * height * depth * bytesFormat->cpuBitsPerPixel / 8.0f);

                    // Allocate space for the converted image
                    face[f] = System::alignedMalloc(numBytes, 16);

                    // Copy the original data
                    System::memcpy(const_cast<void*>(face[f]), _bytes[m][f], numBytes);

                    // Apply the processing to the copy
                    modulateImage
                        (bytesFormat->code,
                         const_cast<void*>(face[f]),
                         numBytes,
                         preprocess.modulate,
                         preprocess.gammaAdjust);
                }
            }
        }
    }

    if (preprocess.computeNormalMap) {
        debugAssertM(bytesFormat->redBits == 8 || bytesFormat->luminanceBits == 8, "To preprocess a texture with normal maps, 8-bit channels are required");
        debugAssertM(bytesFormat->compressed == false, "Cannot compute normal maps from compressed textures");
        debugAssertM(bytesFormat->floatingPoint == false, "Cannot compute normal maps from floating point textures");
        debugAssertM(bytesFormat->numComponents == 1 || bytesFormat->numComponents == 3 || bytesFormat->numComponents == 4, "1, 3, or 4 channels needed to compute normal maps");
        debugAssertM(bytesPtr->size() == 1, "Cannot specify mipmaps when computing normal maps automatically");

        normal = BumpMap::computeNormalMap(width, height, bytesFormat->numComponents, 
                                 reinterpret_cast<const unorm8*>((*bytesPtr)[0][0]),
                                 preprocess.bumpMapPreprocess);
        
        // Replace the previous array with the data from our normal map
        bytesPtr = new MipArray();
        bytesPtr->resize(1);
        (*bytesPtr)[0].append(normal->buffer());
        
        bytesFormat = ImageFormat::RGBA8();

        if (desiredFormat == ImageFormat::AUTO()) {
            desiredFormat = ImageFormat::RGBA8();
        }

        debugAssertM(desiredFormat->openGLBaseFormat == GL_RGBA, "Desired format must contain RGBA channels for bump mapping");
    }

    if (desiredFormat == ImageFormat::AUTO()) {
        desiredFormat = bytesFormat;
    }

    if (settings.interpolateMode != NEAREST_NO_MIPMAP &&
        settings.interpolateMode != NEAREST_MIPMAP) {
        debugAssertM(
                 (desiredFormat->openGLFormat != GL_RGBA32UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA32I) &&
                 (desiredFormat->openGLFormat != GL_RGBA16UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA16I) &&
                 (desiredFormat->openGLFormat != GL_RGBA8UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA8I) &&
                 
                 (desiredFormat->openGLFormat != GL_RGB32UI) &&
                 (desiredFormat->openGLFormat != GL_RGB32I) &&
                 (desiredFormat->openGLFormat != GL_RGB16UI) &&
                 (desiredFormat->openGLFormat != GL_RGB16I) &&
                 (desiredFormat->openGLFormat != GL_RGB8UI) &&
                 (desiredFormat->openGLFormat != GL_RGB8I) &&
                 
                 (desiredFormat->openGLFormat != GL_RG32UI) &&
                 (desiredFormat->openGLFormat != GL_RG32I) &&
                 (desiredFormat->openGLFormat != GL_RG16UI) &&
                 (desiredFormat->openGLFormat != GL_RG16I) &&
                 (desiredFormat->openGLFormat != GL_RG8UI) &&
                 (desiredFormat->openGLFormat != GL_RG8I) &&
                 
                 (desiredFormat->openGLFormat != GL_R32UI) &&
                 (desiredFormat->openGLFormat != GL_R32I) &&
                 (desiredFormat->openGLFormat != GL_R16UI) &&
                 (desiredFormat->openGLFormat != GL_R16I) &&
                 (desiredFormat->openGLFormat != GL_R8UI) &&
                 (desiredFormat->openGLFormat != GL_R8I),
                 
                 "Integer and unsigned integer formats only support NEAREST interpolation");
    }
    debugAssert(bytesFormat);
    (void)depth;
    
    // Check for at least one miplevel on the incoming data
    int numMipMaps = bytesPtr->length();
    debugAssert( numMipMaps > 0 );

    // Create the texture
    GLuint textureID = newGLTextureID();
    GLenum target = dimensionToTarget(dimension);

    if ((desiredFormat == ImageFormat::AUTO()) || (bytesFormat->compressed)) {
        desiredFormat = bytesFormat;
    }

    if (GLCaps::hasBug_redBlueMipmapSwap() && (desiredFormat == ImageFormat::RGB8())) {
        desiredFormat = ImageFormat::RGBA8();
    }

    debugAssertM(GLCaps::supportsTexture(desiredFormat), "Unsupported texture format.");

    glStatePush();
 
        // Set unpacking alignment
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glEnable(target);
        glBindTexture(target, textureID);
        debugAssertGLOk();
        if (isMipMapFormat(settings.interpolateMode) && hasAutoMipMap() && (numMipMaps == 1)) {
            // Enable hardware MIP-map generation.
            // Must enable before setting the level 0 image (we'll set it again
            // in setTexParameters, but that is intended primarily for 
            // the case where that function is called for a pre-existing GL texture ID).
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        }

        int mipWidth = width;
        int mipHeight = height;
        Color4 minval = Color4::nan();
        Color4 meanval = Color4::nan();
        Color4 maxval = Color4::nan();
        for (int mipLevel = 0; mipLevel < numMipMaps; ++mipLevel) {

            const int numFaces = (*bytesPtr)[mipLevel].length();
            
            debugAssert( (((dimension == DIM_CUBE_MAP) || (dimension == DIM_CUBE_MAP_NPOT)) ? 6 : 1) == numFaces);
        
            for (int f = 0; f < numFaces; ++f) {
        
                // Test for both DIM_CUBE_MAP and DIM_CUBE_MAP_NPOT
                if (numFaces == 6) {
                    // Choose the appropriate face target
                    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + f;
                }

                if (isMipMapFormat(settings.interpolateMode) && ! hasAutoMipMap() && (numMipMaps == 1)) {

                    debugAssertM((bytesFormat->compressed == false), 
                                 "Cannot manually generate Mip-Maps for compressed textures.");

                    createMipMapTexture(target, 
                                        reinterpret_cast<const uint8*>((*bytesPtr)[mipLevel][f]),
                                        bytesFormat->openGLFormat,
                                        bytesFormat->openGLBaseFormat,
                                        mipWidth, 
                                        mipHeight, 
                                        desiredFormat->openGLFormat,
                                        bytesFormat->cpuBitsPerPixel / 8, 
                                        scaleFactor,
                                        bytesFormat->openGLDataFormat,
                                        preprocess.computeMinMaxMean,
                                        minval, maxval, meanval);
                    
                } else {

                    const bool useNPOT = (dimension == DIM_2D_NPOT) || (dimension == DIM_CUBE_MAP_NPOT) || (dimension == DIM_3D_NPOT);

                    debugAssertGLOk();
                    createTexture(target, 
                                  reinterpret_cast<const uint8*>((*bytesPtr)[mipLevel][f]), 
                                  bytesFormat->openGLFormat, 
                                  bytesFormat->openGLBaseFormat,
                                  mipWidth, 
                                  mipHeight, 
                                  depth,
                                  desiredFormat->openGLFormat, 
                                  bytesFormat->cpuBitsPerPixel / 8, 
                                  mipLevel, 
                                  bytesFormat->compressed, 
                                  useNPOT, 
                                  scaleFactor,
                                  bytesFormat->openGLDataFormat,
                                  preprocess.computeMinMaxMean,
                                  minval, maxval, meanval);
                }
                debugAssertGLOk();

#               ifndef G3D_DEBUG
                {
                    GLenum e = glGetError();
                    if (e == GL_OUT_OF_MEMORY) {
                        throw std::string("The texture map was too large (GL_OUT_OF_MEMORY)");
                    }
                    if (e != GL_NO_ERROR) {
                    std::string errors;
                    while (e != GL_NO_ERROR) {
                        e = glGetError();
                        if (e == GL_OUT_OF_MEMORY) {
                            throw std::string("The texture map was too large (GL_OUT_OF_MEMORY)");
                        }
                    }
                    }
                }
#               endif
            }

            mipWidth = iMax(1, mipWidth / 2);
            mipHeight = iMax(1, mipHeight / 2);
        }
   
    glStatePop();

    if ((dimension != DIM_2D_RECT) &&
        (dimension != DIM_2D_NPOT) && 
        (dimension != DIM_3D_NPOT) &&
        (dimension != DIM_CUBE_MAP_NPOT)) {
        width  = ceilPow2(width);
        height = ceilPow2(height);
        depth  = ceilPow2(depth);
    }

    debugAssertGLOk();
    Texture::Ref t = fromGLTexture(name, textureID, desiredFormat, dimension, settings);
    debugAssertGLOk();

    t->m_width  = width;
    t->m_height = height;
    t->m_depth  = depth;
    t->m_min    = minval;
    t->m_max    = maxval;
    t->m_mean   = meanval;

    if (bytesPtr != &_bytes) {

        // We must free our own data
        if (normal.notNull()) {
            // The normal ImageBuffer is holding the data; do not free it because 
            // the destructor will do so at the end of the method automatically.
        } else {
            for (int m = 0; m < bytesPtr->size(); ++m) {
                Array<const void*>& face = (*bytesPtr)[m]; 
                for (int f = 0; f < face.size(); ++f) {
                    System::alignedFree(const_cast<void*>(face[f]));
                }
            }
        }
        delete bytesPtr;
        bytesPtr = NULL;
    }

    return t;
}


Texture::Ref Texture::fromImageBuffer(
    const std::string&              name,
    const ImageBuffer::Ref&         image,
    const ImageFormat*              desiredFormat,
    Dimension                       dimension,
    const Settings&                 settings,
    const Preprocess&               preprocess) {

    Array< Array< ImageBuffer::Ref > > mips;
    mips.append( Array< ImageBuffer::Ref >() );
    mips[0].append(image);

    if (desiredFormat == ImageFormat::AUTO()) {
        desiredFormat = image->format();
    }

    Texture::Ref t =
        fromMemory(
            name, 
            image->buffer(), 
            image->format(),
            image->width(), 
            image->height(), 
            image->depth(),
            desiredFormat, 
            dimension, 
            settings,
            preprocess);

    return t;
}

Texture::Ref Texture::createEmpty
(const std::string&               name,
 int                              w,
 int                              h,
 const ImageFormat*               desiredFormat,
 Dimension                        dimension,
 const Settings&                  settings,
 int                              d) {

    debugAssertGLOk();
    debugAssertM(desiredFormat, "desiredFormat may not be ImageFormat::AUTO()");

    if (dimension != DIM_3D && dimension != DIM_3D_NPOT) {
        debugAssertM(d == 1, "Depth must be 1 for DIM_2D textures");
    }

    Texture::Ref t;

    if ((dimension == DIM_CUBE_MAP) || (dimension == DIM_CUBE_MAP_NPOT)) {
        // Cube map requires six faces
        Array< Array<const void*> > data;
        data.resize(1);
        data[0].resize(6);
        for (int i = 0; i < 6; ++i) {
            data[0][i] = NULL;
        }
        t = fromMemory
            (name, 
             data,
             desiredFormat, 
             w, 
             h, 
             d, 
             desiredFormat,
             dimension,
             settings);
    } else {

        t = fromMemory
            (name, 
             NULL, 
             desiredFormat, 
             w, 
             h, 
             d, 
             desiredFormat, 
             dimension, 
             settings);
    }

    if (desiredFormat->depthBits > 0) {
        t->visualization = Visualization::depthBuffer();
    }

    if (isMipMapFormat(settings.interpolateMode)) {
        // Some GPU drivers will not allocate the MIP levels until
        // this is called explicitly, which can cause framebuffer
        // calls to fail
        t->generateMipMaps();
    }

    debugAssertGLOk();
    return t;
}


void Texture::resize(int w, int h) {
    if ((width() == w) && (height() == h)) {
        return;
    }
    m_sizeOfAllTexturesInMemory -= sizeInMemory();
    
    m_min  = Color4::nan();
    m_max  = Color4::nan();
    m_mean = Color4::nan();

    m_width = w;
    m_height = h;
    alwaysAssertM(m_dimension != DIM_CUBE_MAP && m_dimension != DIM_CUBE_MAP_NPOT, "Cannot resize cube map textures");
    Array<GLenum> targets;
    if (m_dimension == DIM_CUBE_MAP || m_dimension == DIM_CUBE_MAP_NPOT) {
        targets.append(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                        GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB);
        targets.append(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB);        
    } else {
        targets.append(openGLTextureTarget());
    }

    glStatePush();
    {
        glBindTexture(openGLTextureTarget(), m_textureID);
        for (int t = 0; t < targets.size(); ++t) {
            glTexImage2D(targets[t], 0, format()->openGLFormat, w, h,
                                 0, format()->openGLBaseFormat, GL_UNSIGNED_BYTE, NULL);
        }
    }
    glStatePop();

    m_sizeOfAllTexturesInMemory += sizeInMemory();
}


void Texture::clear(CubeFace cf, int mipLevel, RenderDevice* rd) {
    if (rd == NULL) {
        rd = RenderDevice::lastRenderDeviceCreated;
    }

    const Framebuffer::Ref& fbo = workingFramebuffer();

    if (m_format->depthBits > 0) {
        fbo->set(Framebuffer::DEPTH, Texture::Ref(this), cf, mipLevel);
    } else {
        fbo->set(Framebuffer::COLOR0, Texture::Ref(this), cf, mipLevel);
    }

    rd->pushState(fbo);
    rd->clear();
    rd->popState();

    fbo->clear();
}


Rect2D Texture::rect2DBounds() const {
    return Rect2D::xywh(0, 0, (float)m_width, (float)m_height);
}


const Texture::Settings& Texture::settings() const {
    return m_settings;
}


void Texture::getImage(ImageBuffer::Ref& im, const ImageFormat* outFormat, CubeFace face) const {
    alwaysAssertM(outFormat == ImageFormat::AUTO() ||
                  outFormat == ImageFormat::RGB8() ||
                  outFormat == ImageFormat::RGBA8() ||
                  outFormat == ImageFormat::L8() ||
                  outFormat == ImageFormat::A8(), "Illegal texture format.");

    if (outFormat == ImageFormat::AUTO()) {
        switch(format()->openGLBaseFormat) { 
        case GL_ALPHA:
            outFormat = ImageFormat::A8();
            break;

        case GL_LUMINANCE:
            outFormat = ImageFormat::L8();
            break;

        case GL_RGB:
            outFormat = ImageFormat::RGB8();
            break;

        case GL_RGBA:
            // Fall through intentionally
        default:
            outFormat = ImageFormat::RGBA8();
            break;
        }
    }

    im = ImageBuffer::create(m_width, m_height, outFormat);

    getTexImage(im->buffer(), outFormat, face);
}


void Texture::getTexImage(void* data, const ImageFormat* desiredFormat, CubeFace face, int mipLevel) const {
    
    GLenum target;
    if (isCubeMap()) { 
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + (int)face;
    } else {
        // Not a cubemap
        target = openGLTextureTarget();
    }

    glPushAttrib(GL_TEXTURE_BIT); {
    glBindTexture(openGLTextureTarget(), openGLID());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
        
    glGetTexImage
        (target,
         mipLevel,
         desiredFormat->openGLBaseFormat,
         desiredFormat->openGLDataFormat,
         data);

    } glPopAttrib();

}


Image4Ref Texture::toImage4() const {
    Image4Ref im = Image4::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::RGBA32F());
    return im;
}

Image4unorm8Ref Texture::toImage4unorm8() const {
    Image4unorm8::Ref im = Image4unorm8::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::RGBA8());
    return im;
}

Image3Ref Texture::toImage3() const {    
    Image3::Ref im = Image3::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::RGB32F());
    return im;
}

Image3unorm8Ref Texture::toImage3unorm8() const {    
    Image3unorm8::Ref im = Image3unorm8::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::RGB8());
    return im;
}

Map2D<float>::Ref Texture::toDepthMap() const {
    Map2D<float>::Ref im = Map2D<float>::create(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::DEPTH32F());
    return im;
}

Image1Ref Texture::toDepthImage1() const {
    Image1Ref im = Image1::createEmpty(m_width, m_height, m_settings.wrapMode);
    getTexImage(im->getCArray(), ImageFormat::DEPTH32F());
    return im;
}

Image1unorm8Ref Texture::toDepthImage1unorm8() const {
    Image1Ref src = toDepthImage1();
    Image1unorm8Ref dst = Image1unorm8::createEmpty(m_width, m_height, m_settings.wrapMode);
    
    const Color1* s = src->getCArray();
    Color1unorm8* d = dst->getCArray();
    
    // Float to int conversion
    for (int i = m_width * m_height - 1; i >= 0; --i) {
        d[i] = Color1unorm8(s[i]);
    }
    
    return dst;
}


Image1Ref Texture::toImage1() const {
    Image1Ref im = Image1::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::L32F());
    return im;
}


Image1unorm8Ref Texture::toImage1unorm8() const {
    Image1unorm8Ref im = Image1unorm8::createEmpty(m_width, m_height, m_settings.wrapMode); 
    getTexImage(im->getCArray(), ImageFormat::L8());
    return im;
}


void Texture::splitFilenameAtWildCard(
    const std::string&  filename,
    std::string&        filenameBase,
    std::string&        filenameExt) {

    const std::string splitter = "*";

    size_t i = filename.rfind(splitter);
    if (i != std::string::npos) {
        filenameBase = filename.substr(0, i);
        filenameExt  = filename.substr(i + 1, filename.size() - i - splitter.length()); 
    } else {
        throw Image::Error("Cube map filenames must contain \"*\" as a "
            "placeholder for {up,lf,rt,bk,ft,dn} or {up,north,south,east,west,down}", filename);
    }
}


bool Texture::isSupportedImage(const std::string& filename) {
    // Reminder: this looks in zipfiles as well
    if (! FileSystem::exists(filename)) {
        return false;
    }

    std::string ext = toLower(filenameExt(filename));

    if ((ext == "jpg") ||    
        (ext == "ico") ||
        (ext == "dds") ||
        (ext == "png") ||
        (ext == "tga") || 
        (ext == "bmp") ||
        (ext == "ppm") ||
        (ext == "pgm") ||
        (ext == "pbm") ||
        (ext == "tiff") ||
        (ext == "exr") ||
        (ext == "cut") ||
        (ext == "psd") ||
        (ext == "jbig") ||
        (ext == "xbm") ||
        (ext == "xpm") ||
        (ext == "gif") ||
        (ext == "hdr") ||
        (ext == "iff") ||
        (ext == "jng") ||
        (ext == "pict") ||
        (ext == "ras") ||
        (ext == "wbmp") ||
        (ext == "sgi") ||
        (ext == "pcd") ||
        (ext == "jp2") ||
        (ext == "jpx") ||
        (ext == "jpf") ||
        (ext == "pcx")) {
        return true;
    } else {
        return false;
    }
}


Texture::~Texture() {
    m_sizeOfAllTexturesInMemory -= sizeInMemory();
    glDeleteTextures(1, &m_textureID);
    m_textureID = 0;
}


unsigned int Texture::newGLTextureID() {
    // Clear the OpenGL error flag
#   ifdef G3D_DEBUG
        glGetError();
#   endif 

    unsigned int id;
    glGenTextures(1, &id);

    debugAssertM(glGetError() != GL_INVALID_OPERATION, 
         "GL_INVALID_OPERATION: Probably caused by invoking "
         "glGenTextures between glBegin and glEnd.");

    return id;
}

void Texture::copyFromScreen(const Rect2D& rect, const ImageFormat* fmt) {
    glStatePush();

    m_sizeOfAllTexturesInMemory -= sizeInMemory();

    if (fmt == NULL) {
        fmt = format();
    } else {
        m_format = fmt;
    }

    // Set up new state
    m_width   = (int)rect.width();
    m_height  = (int)rect.height();
    m_depth   = 1;
    debugAssert(m_dimension == DIM_2D || m_dimension == DIM_2D_RECT || m_dimension == DIM_2D_NPOT);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    glDisableAllTextures();
    GLenum target = dimensionToTarget(m_dimension);
    glEnable(target);

    debugAssertGLOk();
    glBindTexture(target, m_textureID);
    debugAssertGLOk();

    glCopyTexImage2D(target, 0, format()->openGLFormat,
                     iRound(rect.x0()), 
                     iRound(rect.y0()), 
                     iRound(rect.width()), 
                     iRound(rect.height()), 
                     0);

    debugAssertGLOk();
    // Reset the original properties
    setTexParameters(target, m_settings);

    debugAssertGLOk();
    glDisable(target);

    glStatePop();

    m_sizeOfAllTexturesInMemory += sizeInMemory();
}



void Texture::copyFromScreen(
    const Rect2D&       rect,
    CubeFace            face) {

    glStatePush();

    // Set up new state
    debugAssertM(m_width == rect.width(), "Cube maps require all six faces to have the same dimensions");
    debugAssertM(m_height == rect.height(), "Cube maps require all six faces to have the same dimensions");
    debugAssert(m_dimension == DIM_CUBE_MAP || m_dimension == DIM_CUBE_MAP_NPOT);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    glDisableAllTextures();

    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_textureID);

    GLenum target = openGLTextureTarget();
    if (isCubeMap()) {
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face;
    }

    debugAssertGLOk();

    double viewport[4];
    glGetDoublev(GL_VIEWPORT, viewport);
    double viewportHeight = viewport[3];
    debugAssertGLOk();

    glCopyTexImage2D(target, 0, format()->openGLFormat,
                     iRound(rect.x0()), 
                     iRound(viewportHeight - rect.y1()), 
                     iRound(rect.width()), 
                     iRound(rect.height()), 0);

    debugAssertGLOk();
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    glStatePop();
}


void Texture::getCubeMapRotation(CubeFace face, Matrix3& outMatrix) {
    switch (face) {
    case CubeFace::POS_X:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)halfPi());
        break;
        
    case CubeFace::NEG_X:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)-halfPi());
        break;
        
    case CubeFace::POS_Y:
        outMatrix = CFrame::fromXYZYPRDegrees(0,0,0,180,-90,0).rotation;
        break;
        
    case CubeFace::NEG_Y:
        outMatrix = CFrame::fromXYZYPRDegrees(0,0,0,180,90,0).rotation;
        break;
        
    case CubeFace::POS_Z:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)pi());
        break;
        
    case CubeFace::NEG_Z:
        outMatrix = Matrix3::identity();
        break;

    default:
        alwaysAssertM(false, "");
    }
    
    // GL's cube maps are "inside out" (they are the outside of a box,
    // not the inside), but its textures are also upside down, so
    // these turn into a 180-degree rotation, which fortunately does
    // not affect the winding direction.
    outMatrix = Matrix3::fromAxisAngle(Vector3::unitZ(), toRadians(180)) * outMatrix;
}


int Texture::sizeInMemory() const {

    int64 base = (m_width * m_height * m_depth * m_format->openGLBitsPerPixel) / 8;

    int64 total = 0;

    if (m_settings.interpolateMode == TRILINEAR_MIPMAP) {
        int w = m_width;
        int h = m_height;

        while ((w > 2) && (h > 2)) {
            total += base;
            base /= 4;
            w /= 2;
            h /= 2;
        }

    } else {
        total = base;
    }

    if (m_dimension == DIM_CUBE_MAP) {
        total *= 6;
    }

    return total;
}


unsigned int Texture::openGLTextureTarget() const {
    switch (m_dimension) {
    case DIM_CUBE_MAP:
    case DIM_CUBE_MAP_NPOT:
        return GL_TEXTURE_CUBE_MAP_ARB;

    case DIM_2D:
    case DIM_2D_NPOT:
        return GL_TEXTURE_2D;

    case Texture::DIM_2D_RECT:
        return GL_TEXTURE_RECTANGLE_EXT;

    case Texture::DIM_3D:
    case Texture::DIM_3D_NPOT:
        return GL_TEXTURE_3D;

    default:
        debugAssertM(false, "Fell through switch");
    }
    return 0;
}


Texture::Ref Texture::alphaOnlyVersion() const {
    if (opaque()) {
        return NULL;
    }

    debugAssert(m_settings.depthReadMode == DEPTH_NORMAL);
    debugAssertM(
        m_dimension == DIM_2D ||
        m_dimension == DIM_2D_RECT ||
        m_dimension == DIM_2D_NPOT,
        "alphaOnlyVersion only supported for 2D textures");
    
    int numFaces = 1;

    Array< Array<const void*> > mip;
    mip.resize(1);
    Array<const void*>& bytes = mip[0];
    bytes.resize(numFaces);
    const ImageFormat* bytesFormat = ImageFormat::A8();

    glStatePush();
    // Setup to later implement cube faces
    for (int f = 0; f < numFaces; ++f) {
        GLenum target = dimensionToTarget(m_dimension);
        glBindTexture(target, m_textureID);
        bytes[f] = (const void*)System::malloc(m_width * m_height);
        glGetTexImage(target, 0, GL_ALPHA, GL_UNSIGNED_BYTE, const_cast<void*>(bytes[f]));
    }

    glStatePop();

    Texture::Ref ret = 
        fromMemory(
            m_name + " Alpha", 
            mip,
            bytesFormat,
            m_width, 
            m_height, 
            1, 
            ImageFormat::A8(),
            m_dimension, 
            m_settings);

    for (int f = 0; f < numFaces; ++f) {
        System::free(const_cast<void*>(bytes[f]));
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////
void Texture::setDepthReadMode(Texture::DepthReadMode depthReadMode) {
    if (m_settings.depthReadMode != depthReadMode) {
        m_settings.depthReadMode = depthReadMode;
        glStatePush();
        glBindTexture(openGLTextureTarget(), openGLID());
        setDepthTexParameters(openGLTextureTarget(), depthReadMode);
        glStatePop();
        debugAssertGLOk();
    }
}


void Texture::setDepthTexParameters(GLenum target, Texture::DepthReadMode depthReadMode) {
    if (GLCaps::supports_GL_ARB_shadow()) {
        if (depthReadMode == Texture::DEPTH_NORMAL) {

            glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
            glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);

        } else {

            glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
            glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
            glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC_ARB, 
                (depthReadMode == Texture::DEPTH_LEQUAL) ? GL_LEQUAL : GL_GEQUAL);
        }
    }
}


void Texture::setTexParameters
(GLenum                          target,
 const Texture::Settings&        settings) {

    debugAssert(
        target == GL_TEXTURE_2D ||
        target == GL_TEXTURE_RECTANGLE_EXT ||
        target == GL_TEXTURE_CUBE_MAP ||
        target == GL_TEXTURE_3D);

    debugAssertGLOk();

    // Set the wrap and interpolate state

    GLenum mode = GL_NONE;
    
    switch (settings.wrapMode) {
    case WrapMode::TILE:
        mode = GL_REPEAT;
        break;

    case WrapMode::CLAMP:  
        if (GLCaps::supports_GL_EXT_texture_edge_clamp()) {
            mode = GL_CLAMP_TO_EDGE;
        } else {
            mode = GL_CLAMP;
        }
        break;

    case WrapMode::ZERO:
        if (GLCaps::supports_GL_ARB_texture_border_clamp()) {
            mode = GL_CLAMP_TO_BORDER_ARB;
        } else {
            mode = GL_CLAMP;
        }
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<const float*>(&Color4::clear()));
        debugAssertGLOk();
        break;

    default:
        debugAssertM(Texture::supportsWrapMode(settings.wrapMode), "Unsupported wrap mode for Texture");
    }
    
    glTexParameteri(target, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, mode);
    glTexParameteri(target, GL_TEXTURE_WRAP_R, mode);

    debugAssertGLOk();

    bool hasMipMaps = 
        (target != GL_TEXTURE_RECTANGLE_EXT) &&
        (settings.interpolateMode != Texture::BILINEAR_NO_MIPMAP) &&
        (settings.interpolateMode != Texture::NEAREST_NO_MIPMAP);

    if (hasMipMaps &&
        (GLCaps::supports("GL_EXT_texture_lod") || 
         GLCaps::supports("GL_SGIS_texture_lod"))) {

        // I can find no documentation for GL_EXT_texture_lod even though many cards claim to support it - Morgan 6/30/06
        glTexParameteri(target, GL_TEXTURE_MAX_LOD_SGIS, settings.maxMipMap);
        glTexParameteri(target, GL_TEXTURE_MIN_LOD_SGIS, settings.minMipMap);
    }


    switch (settings.interpolateMode) {
    case Texture::TRILINEAR_MIPMAP:
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        if (hasAutoMipMap()) {  
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        }
        break;

    case Texture::BILINEAR_MIPMAP:
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

        if (hasAutoMipMap() && settings.autoMipMap) {  
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        }
        break;

    case Texture::NEAREST_MIPMAP:
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

        if (hasAutoMipMap() && settings.autoMipMap) {  
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        }
        break;

    case Texture::BILINEAR_NO_MIPMAP:
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
        break;

    case Texture::NEAREST_NO_MIPMAP:
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;

    default:
        debugAssert(false);
    }
    debugAssertGLOk();

    static const bool anisotropic = GLCaps::supports("GL_EXT_texture_filter_anisotropic");

    if (anisotropic) {
        glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, settings.maxAnisotropy);
    }

    Texture::setDepthTexParameters(target, settings.depthReadMode);
    debugAssertGLOk();
}

static void glStatePush() {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
}


static bool hasAutoMipMap() {
    static bool initialized = false;
    static bool ham = false;

    if (! initialized) {
        initialized = true;
        ham = GLCaps::supports_GL_SGIS_generate_mipmap() &&
            ! GLCaps::hasBug_mipmapGeneration() &&
            ! GLCaps::hasBug_redBlueMipmapSwap();
    }

    return ham;
}


static GLenum dimensionToTarget(Texture::Dimension d) {
    switch (d) {
    case Texture::DIM_CUBE_MAP:
    case Texture::DIM_CUBE_MAP_NPOT:
        return GL_TEXTURE_CUBE_MAP_ARB;
        
    case Texture::DIM_2D:
    case Texture::DIM_2D_NPOT:
        return GL_TEXTURE_2D;

    case Texture::DIM_2D_RECT:
        return GL_TEXTURE_RECTANGLE_EXT;

    case Texture::DIM_3D:
    case Texture::DIM_3D_NPOT:
        return GL_TEXTURE_3D;

    default:
        debugAssert(false);
        return GL_TEXTURE_2D;
    }
}



void computeStats
(const uint8* rawBytes, 
 GLenum       bytesActualFormat, 
 int          width,
 int          height,
 Color4&      minval,
 Color4&      maxval,
 Color4&      meanval) {
    minval  = Color4::nan();
    maxval  = Color4::nan();
    meanval = Color4::nan();
    if (rawBytes == NULL) {
        return;
    }

    float inv255Width = 1.0f / (width * 255);
    switch (bytesActualFormat) {
    case GL_RGB8:
        {
            Color3unorm8 mn = Color3unorm8::one();
            Color3unorm8 mx = Color3unorm8::zero();
            meanval = Color4::zero();
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color3unorm8* ptr = ((const Color3unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0;
                for (int x = 0; x < width; ++x) {
                    const Color3unorm8 i = ptr[x];
                    mn = mn.min(i);
                    mx = mx.max(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits();
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, 1.0);
            }
            minval  = Color4(Color3(mn), 1.0f);
            maxval  = Color4(Color3(mx), 1.0f);
            meanval /= height;
        }
        break;

    case GL_RGBA8:
        {
            // TODO: rewrite using SIMD PMAXUB and PMINUB to try and increase performance

            meanval = Color4::zero();
            Color4unorm8 mn = Color4unorm8::one();
            Color4unorm8 mx = Color4unorm8::zero();
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color4unorm8* ptr = ((const Color4unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0, a = 0;
                for (int x = 0; x < width; ++x) {
                    const Color4unorm8 i = ptr[x];
                    mn = mn.min(i);
                    mx = mx.min(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits(); a += i.a.bits();
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, float(a) * inv255Width);
            }
            minval = mn;
            maxval = mx;
            meanval = meanval / height;
        }
        break;

    default:
        break;
    }
}

/** 
   @param bytesFormat OpenGL base format.

   @param bytesActualFormat OpenGL true format.  For compressed data,
   distinguishes the format that the data has due to compression.
 
   @param dataType Type of the incoming data from the CPU, e.g. GL_UNSIGNED_BYTES 
*/
static void createTexture(
    GLenum          target,
    const uint8*    rawBytes,
    GLenum          bytesActualFormat,
    GLenum          bytesFormat,
    int             m_width,
    int             m_height,
    int             depth,
    GLenum          ImageFormat,
    int             bytesPerPixel,
    int             mipLevel,
    bool            compressed,
    bool            useNPOT,
    float           rescaleFactor,
    GLenum          dataType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval) {

    uint8* bytes = const_cast<uint8*>(rawBytes);

    // If true, we're supposed to free the byte array at the end of
    // the function.
    bool   freeBytes = false; 
    int maxSize = GLCaps::maxTextureSize();
    if (computeMinMaxMean) {
        computeStats(rawBytes, bytesActualFormat, m_width, m_height, minval, maxval, meanval);
    }

    switch (target) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        maxSize = GLCaps::maxCubeMapSize();

        // Fall through

    case GL_TEXTURE_2D:
        if ((rescaleFactor != 1.0) || 
            ( (! isPow2(m_width) || ! isPow2(m_height)) &&
              (! useNPOT || ! GLCaps::supports_GL_ARB_texture_non_power_of_two() ||
              (m_width > maxSize) || (m_height > maxSize)) )) {
            // NPOT texture with useNPOT disabled: resize to a power of two

            debugAssertM(! compressed,
                "Cannot rescale compressed textures to power of two, use DIM_2D_NPOT");


            if (rawBytes != NULL) {
                int oldWidth = m_width;
                int oldHeight = m_height;
                m_width  = iMin(maxSize, ceilPow2(static_cast<unsigned int>(m_width * rescaleFactor)));
                m_height = iMin(maxSize, ceilPow2(static_cast<unsigned int>(m_height * rescaleFactor)));

                if ((oldWidth > maxSize) || (oldHeight > maxSize)) {
                    logPrintf("WARNING: %d x %d texture exceeded maximum size and was resized to %d x %d\n",
                              oldWidth, oldHeight, m_width, m_height);
                }


                bytes = new uint8[m_width * m_height * bytesPerPixel];
                freeBytes = true;

                // Rescale the image to a power of 2
                gluScaleImage(
                    bytesFormat,
                    oldWidth,
                    oldHeight,
                    dataType,
                    rawBytes,
                    m_width,
                    m_height,
                    dataType,
                    bytes);
            }
        }

        // Intentionally fall through for power of 2 case

    case GL_TEXTURE_RECTANGLE_EXT:

        // Note code falling through from above

        if (compressed) {
            
            debugAssertM((target != GL_TEXTURE_RECTANGLE_EXT),
                "Compressed textures must be DIM_2D or DIM_2D_NPOT.");

            glCompressedTexImage2DARB
                (target, mipLevel, bytesActualFormat, m_width, 
                 m_height, 0, (bytesPerPixel * ((m_width + 3) / 4) * ((m_height + 3) / 4)), 
                 rawBytes);

        } else {

            if (bytes != NULL) {
                debugAssert(isValidPointer(bytes));
                debugAssertM(isValidPointer(bytes + (m_width * m_height - 1) * bytesPerPixel), 
                    "Byte array in Texture creation was too small");
            }

            // 2D texture, level of detail 0 (normal), internal
            // format, x size from image, y size from image, border 0
            // (normal), rgb color data, unsigned byte data, and
            // finally the data itself.
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glTexImage2D(target, mipLevel, ImageFormat, m_width, m_height, 0, bytesFormat, dataType, bytes);                         
        }
        break;

    case GL_TEXTURE_3D:
        // Can't rescale, so ensure that the texture is a power of two in each dimension
        debugAssertM(
            (useNPOT && GLCaps::supports_GL_ARB_texture_non_power_of_two()) ||
            (isPow2(m_width) && isPow2(m_height) && isPow2(depth)),
                     "DIM_3D textures must be a power of two size in each dimension");

        if (bytes != NULL) {
            debugAssert(isValidPointer(bytes));
        }

        glTexImage3DEXT(target, mipLevel, ImageFormat, m_width, m_height, depth, 0, 
                        bytesFormat, dataType, bytes);
        break;

    default:
        debugAssertM(false, "Fell through switch");
    }

    if (freeBytes) {
        // Texture was resized; free the temporary.
        delete[] bytes;
    }
}


static void createMipMapTexture(    
    GLenum          target,
    const uint8*    _bytes,
    int             bytesFormat,
    int             bytesBaseFormat,
    int             m_width,
    int             m_height,
    GLenum          desiredFormat,
    int             bytesFormatBytesPerPixel,
    float           rescaleFactor,
    GLenum          bytesType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval) {

    if (computeMinMaxMean) {
        computeStats(_bytes, bytesFormat, m_width, m_height, minval, maxval, meanval);
    }

    switch (target) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            bool freeBytes = false;
            const uint8* bytes = _bytes;

            if (rescaleFactor != 1.0f) {
                int oldWidth = m_width;
                int oldHeight = m_height;
                m_width  = ceilPow2(static_cast<unsigned int>(m_width * rescaleFactor));
                m_height = ceilPow2(static_cast<unsigned int>(m_height * rescaleFactor));

                bytes = new uint8[m_width * m_height * bytesFormatBytesPerPixel];
                freeBytes = true;

                // Rescale the image to a power of 2

                // http://www.csee.umbc.edu/help/C++/opengl/man_pages/html/glu/scaleimage.html
                gluScaleImage(
                    bytesBaseFormat,
                    oldWidth,
                    oldHeight,
                    bytesType,
                    _bytes,
                    m_width,
                    m_height,
                    bytesType,
                    (void*)bytes);
            }

            alwaysAssertM(false, "CPU MIP-maps were removed in G3D 9.0");
            // Build mip-maps on the CPU (if we had hardware support, createTexture would have been called instead)
            // http://www.opengl.org/sdk/docs/man/xhtml/gluBuild2DMipmaps.xml
            int r = gluBuild2DMipmaps(target, desiredFormat, m_width, m_height, bytesBaseFormat, bytesType, bytes);
            debugAssertM(r == 0, (const char*)gluErrorString(r)); (void)r;

            if (freeBytes) {
                delete[] const_cast<uint8*>(bytes);
            }
            break;
        }

    default:
        debugAssertM(false, "Mipmaps not supported for this texture target");
    }
}


static void modulateImage(ImageFormat::Code fmt, void* _byte, int n, const Color4& modulate, float gamma) {

    debugAssert(
        (fmt == ImageFormat::CODE_RGB8) ||
        (fmt == ImageFormat::CODE_RGBA8) ||
        (fmt == ImageFormat::CODE_R8) ||
        (fmt == ImageFormat::CODE_L8));

    uint8* byte = static_cast<uint8*>(_byte);

    // Make a lookup table
    uint8 adjust[4][256];
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i < 256; ++i) {
            float s = pow((i * modulate[c]) / 255.0f, gamma) * 255;
            adjust[c][i] = iClamp(iRound(s), 0, 255);
        }
    }

    switch (fmt) {
    case ImageFormat::CODE_RGBA8:
        // 4 channels (we duplicate the loop so that it can be unrolled by the compiler)
        for (int i = 0; i < n; ) {
            for (int c = 0; c < 4; ++c, ++i) {
                byte[i] = adjust[c][byte[i]];
            }
        }
        break;

    case ImageFormat::CODE_RGB8:
        for (int i = 0; i < n; ) {
            for (int c = 0; c < 3; ++c, ++i) {
                byte[i] = adjust[c][byte[i]];
            }
        }
        break;

    case ImageFormat::CODE_R8:
    case ImageFormat::CODE_L8:
        for (int i = 0; i < n; ++i) {
            byte[i] = adjust[0][byte[i]];
        }
        break;

    default:;
    }
}


/////////////////////////////////////////////////////
Any Texture::Specification::toAny() const {
    Any a = Any(Any::TABLE, "Texture::Specification");
    a["filename"] = filename;
    a["desiredFormat"] = desiredFormat ? desiredFormat->name() : "AUTO";
    a["dimension"] = toString(dimension);
    a["settings"]  = settings;
    a["preprocess"] = preprocess;
    a["visualization"] = visualization;

    return a;
}


void Texture::Specification::serialize(BinaryOutput& b) const {
    toAny().serialize(b);
}


void Texture::Specification::deserialize(BinaryInput& b) {
    Any a;
    a.deserialize(b);
    *this = a;
}


bool Texture::Specification::operator==(const Specification& other) const {
    return 
        (filename == other.filename) &&
        (desiredFormat == other.desiredFormat) &&
        (dimension == other.dimension) &&
        (settings == other.settings) &&
        (preprocess == other.preprocess) &&
        (visualization == other.visualization);
}


Texture::Specification::Specification(const Any& any) {
    *this = Specification();

    if (any.type() == Any::STRING) {
        filename = any.string();
        if (filename == "<whiteCube>") {
            filename = "<white>";
            settings = Texture::Settings::cubeMap();
        }

        if (! beginsWith(filename, "<")) {
            filename = any.resolveStringAsFilename();
            if (FilePath::containsWildcards(filename)) {
                // Assume this is a cube map
                settings = Texture::Settings::cubeMap();
            }
        }
    } else {
        any.verifyNameBeginsWith("Texture::Specification");
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const std::string& key = it->key;
            if (key == "filename") {
                filename = it->value.resolveStringAsFilename();
            } else if (key == "desiredFormat") {
                desiredFormat = ImageFormat::fromString(it->value.string());
            } else if (key == "dimension") {
                dimension = toDimension(it->value);
            } else if (key == "settings") {
                settings = it->value;
            } else if (key == "preprocess") {
                preprocess = it->value;
            } else if (key == "visualization") {
                visualization = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
        if (! any.containsKey("settings") && FilePath::containsWildcards(filename)) {
            // Assume this is a cube map
            settings = Texture::Settings::cubeMap();
        }
    }
}

const char* Texture::toString(DepthReadMode m) {
    switch (m) {
    case DEPTH_NORMAL: return "DEPTH_NORMAL";
    case DEPTH_LEQUAL: return "DEPTH_LEQUAL";
    case DEPTH_GEQUAL: return "DEPTH_GEQUAL";
    default:
        return "ERROR";
    }
}


Texture::Dimension Texture::toDimension(const std::string& s) {
    if (s == "DIM_2D") {
        return DIM_2D;
    } else if (s == "DIM_2D_NPOT") {
        return DIM_2D_NPOT;
    } else if (s == "DIM_2D_RECT") {
        return DIM_2D_RECT;
    } else if (s == "DIM_3D") {
        return DIM_3D;
    } else if (s == "DIM_3D_NPOT") {
        return DIM_3D_NPOT;
    } else if (s == "DIM_CUBE_MAP") {
        return DIM_CUBE_MAP;
    } else if (s == "DIM_CUBE_MAP_NPOT") {
        return DIM_CUBE_MAP_NPOT;
    } else {
        debugAssertM(false, "Unrecognized dimension");
        return DIM_2D;
    }
}


const char* Texture::toString(Dimension d) {
    switch (d) {
    case DIM_2D: return "DIM_2D";
    case DIM_3D: return "DIM_3D";
    case DIM_2D_RECT: return "DIM_2D_RECT";
    case DIM_CUBE_MAP: return "DIM_CUBE_MAP";
    case DIM_2D_NPOT: return "DIM_2D_NPOT";
    case DIM_CUBE_MAP_NPOT: return "DIM_CUBE_MAP_NPOT";
    case DIM_3D_NPOT: return "DIM_3D_NPOT";
    default:
        return "ERROR";
    }
}


Texture::DepthReadMode Texture::toDepthReadMode(const std::string& s) {
    if (s == "DEPTH_NORMAL") {
        return DEPTH_NORMAL;
    } else if (s == "DEPTH_LEQUAL") {
        return DEPTH_LEQUAL;
    } else if (s == "DEPTH_GEQUAL") {
        return DEPTH_GEQUAL;
    } else {
        debugAssertM(false, "Unrecognized depth read mode");
        return DEPTH_NORMAL;
    }
}


const char* Texture::toString(InterpolateMode m) {
    switch (m) {
    case TRILINEAR_MIPMAP:   return "TRILINEAR_MIPMAP";
    case BILINEAR_MIPMAP:    return "BILINEAR_MIPMAP";
    case NEAREST_MIPMAP:     return "NEAREST_MIPMAP";
    case BILINEAR_NO_MIPMAP: return "BILINEAR_NO_MIPMAP";
    case NEAREST_NO_MIPMAP:  return "NEAREST_NO_MIPMAP";
    default:
        return "ERROR";
    }
}


Texture::InterpolateMode Texture::toInterpolateMode(const std::string& s) {
    if (s == "TRILINEAR_MIPMAP") {
        return TRILINEAR_MIPMAP;
    } else if (s == "BILINEAR_MIPMAP") {
        return BILINEAR_MIPMAP;
    } else if (s == "NEAREST_MIPMAP") {
        return NEAREST_MIPMAP;
    } else if (s == "BILINEAR_NO_MIPMAP") {
        return BILINEAR_NO_MIPMAP;
    } else if (s == "NEAREST_NO_MIPMAP") {
        return NEAREST_NO_MIPMAP;
    } else {
        debugAssertM(false, "Unrecognized interpolate mode");
        return TRILINEAR_MIPMAP;
    }
}


Any Texture::Settings::toAny() const {
    Any a(Any::TABLE, "Texture::Settings");
    a["interpolateMode"] = toString(interpolateMode);
    a["wrapMode"]        = wrapMode.toAny();
    a["maxAnisotropy"]   = maxAnisotropy;
    a["autoMipMap"]      = autoMipMap;
    a["maxMipMap"]       = maxMipMap;
    a["minMipMap"]       = minMipMap;
    return a;
}


Texture::Settings::Settings(const Any& any) {
    *this = Settings::defaults();
    any.verifyNameBeginsWith("Texture::Settings");
    if (any.type() == Any::TABLE) {
        AnyTableReader r(any);

        r.getIfPresent("autoMipMap", autoMipMap);
        Any temp;
        if (r.getIfPresent("depthReadMode", temp)) {
            depthReadMode = toDepthReadMode(temp);
        }
        if (r.getIfPresent("interpolateMode", temp)) {
            interpolateMode = toInterpolateMode(temp);
        }
        r.getIfPresent("maxAnisotropy", maxAnisotropy);
        r.getIfPresent("maxMipMap", maxMipMap);
        r.getIfPresent("minMipMap", minMipMap);
        r.getIfPresent("wrapMode", wrapMode);
        r.verifyDone();
    } else {
        any.verifySize(0);
        const std::string& n = any.name();
        if (n == "Texture::Settings::defaults") {
            // Done!
        } else if (n == "Texture::Settings::buffer") {
            *this = Texture::Settings::buffer();
        } else if (n == "Texture::Settings::cubeMap") {
            *this = Texture::Settings::cubeMap();
        } else if (n == "Texture::Settings::shadow") {
            *this = Texture::Settings::shadow();
        } else if (n == "Texture::Settings::video") {
            *this = Texture::Settings::video();
        } else {
            any.verify(false, "Unrecognized name for Texture::Settings constructor or factory method.");
        }
    }
}

Texture::Settings::Settings() : 
    interpolateMode(TRILINEAR_MIPMAP),
    wrapMode(WrapMode::TILE),
    depthReadMode(DEPTH_NORMAL),
    maxAnisotropy(4.0),
    autoMipMap(true),
    maxMipMap(1000),
    minMipMap(-1000) {
}


const Texture::Settings& Texture::Settings::defaults() {
    static Settings param;
    return param;
}


const Texture::Settings& Texture::Settings::video() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = BILINEAR_NO_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = false;
    }

    return param;
}


const Texture::Settings& Texture::Settings::buffer() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = NEAREST_NO_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = false;
    }

    return param;
}


const Texture::Settings& Texture::Settings::cubeMap() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        param.interpolateMode = BILINEAR_MIPMAP;
        param.wrapMode = WrapMode::CLAMP;
        param.depthReadMode = DEPTH_NORMAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap = true;
    }

    return param;
}


const Texture::Settings& Texture::Settings::shadow() {

    static bool initialized = false;
    static Settings param;

    if (! initialized) {
        initialized = true;
        if (GLCaps::enumVendor() == GLCaps::ATI) {
            // ATI cards do not implement PCF for shadow maps
            param.interpolateMode = NEAREST_NO_MIPMAP;
        } else {
            param.interpolateMode = BILINEAR_NO_MIPMAP;
        }
        param.wrapMode      = WrapMode::ZERO;
        param.depthReadMode = DEPTH_LEQUAL;
        param.maxAnisotropy = 1.0;
        param.autoMipMap    = false;
    }

    return param;
}


size_t Texture::Settings::hashCode() const {
    return 
        (uint32)interpolateMode + 
        16 * (uint32)wrapMode + 
        256 * (uint32)depthReadMode + 
        (autoMipMap ? 512 : 0) +
        (uint32)(1024 * maxAnisotropy) +
        (minMipMap ^ (maxMipMap << 16));
}

bool Texture::Settings::operator==(const Settings& other) const {
    return 
        (interpolateMode == other.interpolateMode) &&
        (wrapMode == other.wrapMode) &&
        (depthReadMode == other.depthReadMode) &&
        (maxAnisotropy == other.maxAnisotropy) &&
        (autoMipMap == other.autoMipMap) &&
        (maxMipMap == other.maxMipMap) &&
        (minMipMap == other.minMipMap);
}

bool Texture::Settings::equalsIgnoringMipMap(const Settings& other) const {
    return 
        (interpolateMode == other.interpolateMode) &&
        (wrapMode == other.wrapMode) &&
        (depthReadMode == other.depthReadMode);
}



} // G3D
