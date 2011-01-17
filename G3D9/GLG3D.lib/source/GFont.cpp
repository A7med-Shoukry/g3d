/**
 \file GFont.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-11-02
 \edited  2011-01-20
 */

#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/ImageFormat.h"
#include "G3D/Vector2.h"
#include "G3D/System.h"
#include "G3D/Array.h"
#include "G3D/fileutils.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/WeakCache.h"
#include "G3D/TextInput.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"

namespace G3D {

static WeakCache<std::string, GFontRef>& fontCache() {
    static WeakCache<std::string, GFontRef> cache;
    return cache;
}


/** Copies blocks of src so that they are centered in the corresponding squares of dst.  Assumes each is a 16x16 grid.*/
void GFont::recenterGlyphs(const GImage& src, GImage& dst) {
    // Set dst to all black
    debugAssert(src.channels() == 3);
    debugAssert(dst.channels() == 3);
    debugAssert(dst.width()  >= src.width());
    debugAssert(dst.height() >= src.height());

    System::memset(dst.byte(), 0, dst.width() * dst.height() * 3);

    // Block sizes
    const int srcWidth  = src.width()  / 16;
    const int srcHeight = src.height() / 16;
    const int dstWidth  = dst.width()  / 16;
    const int dstHeight = dst.height() / 16;

    const int dstOffsetX = (dstWidth  - srcWidth) / 2;
    const int dstOffsetY = (dstHeight - srcHeight) / 2;

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            GImage::pasteSubImage(dst, src, x * dstWidth + dstOffsetX, y * dstHeight + dstOffsetY, x * srcWidth, y * srcHeight, srcWidth, srcHeight);
        }
    }
}


void GFont::adjustINIWidths(const std::string& srcFile, const std::string& dstFile, float scale) {
    TextInput in(srcFile);
    TextOutput out(dstFile);
    in.readSymbols("[", "Char", "Widths", "]");
    out.printf("[Char Widths]\n");
    for (int i = 0; i <= 255; ++i) {
        in.readNumber();
        in.readSymbol("=");

        int w = in.readNumber();
        w = iRound(w * scale);
        out.printf("%d=%d\n", i, w);
    }
    out.commit();
}


GFontRef GFont::fromFile(const std::string& filename) {

    if (! FileSystem::exists(filename)) {
        debugAssertM(false, format("Could not load font: %s", filename.c_str()));
        return NULL;
    }

    std::string key = filenameBaseExt(filename);
    GFontRef font = fontCache()[key];
    if (font.isNull()) {
        BinaryInput b(filename, G3D_LITTLE_ENDIAN, true);
        font = new GFont(filename, b);
        fontCache().set(key, font);
    }

    return font;
}


GFontRef GFont::fromMemory(const std::string& name, const uint8* bytes, const int size) {
    // Note that we do not need to copy the memory since GFont will be done with it
    // by the time this method returns.
    BinaryInput b(bytes, size, G3D_LITTLE_ENDIAN, true, false); 
    return new GFont(name, b);
} 


GFont::GFont(const std::string& filename, BinaryInput& b) : m_texture(NULL) {

    debugAssertM(GLCaps::supportsTexture(ImageFormat::A8()),
        "This graphics card does not support the GL_ALPHA8 texture format used by GFont.");
    debugAssertGLOk();

    int ver = b.readInt32();
    debugAssertM(ver == 1 || ver == 2, "Can't read font files other than version 1");
    (void)ver;

    if (ver == 1) {
        charsetSize = 128;
    } else {
        charsetSize = b.readInt32();
    }

    // Read the widths
    subWidth.resize(charsetSize);
    for (int c = 0; c < charsetSize; ++c) {
        subWidth[c] = b.readUInt16();
    }

    baseline = b.readUInt16();
    int texWidth = b.readUInt16();
    charWidth  = texWidth / 16;
    charHeight = texWidth / 16;

    // The input may not be a power of 2
    int width  = ceilPow2(charWidth * 16);
    int height = ceilPow2(charHeight * (charsetSize / 16));
  
    // Create a texture
    const uint8* ptr = ((uint8*)b.getCArray()) + b.getPosition();
    debugAssertM((b.getLength() - b.getPosition()) >= width * height, "File does not contain enough data for this size texture");

	Texture::Settings fontSettings;
	fontSettings.wrapMode = WrapMode::CLAMP;
    fontSettings.interpolateMode = Texture::TRILINEAR_MIPMAP;
    Texture::Preprocess preprocess;
    preprocess.computeMinMaxMean = false;

    m_texture = 
        Texture::fromMemory(
			filename, 
			ptr,
            ImageFormat::A8(), 
			width, 
			height,
			1,
			ImageFormat::A8(), 
			Texture::DIM_2D,
			fontSettings,
            preprocess);
   
    m_textureMatrix[0] = 1.0f / m_texture->width();
    m_textureMatrix[1] = 0;
    m_textureMatrix[2] = 0;
    m_textureMatrix[3] = 0;
    m_textureMatrix[4] = 0;
    m_textureMatrix[5] = 1.0f / m_texture->height();
    m_textureMatrix[6] = 0;
    m_textureMatrix[7] = 0;    
    m_textureMatrix[8] = 0;
    m_textureMatrix[9] = 0;
    m_textureMatrix[10] = 1;
    m_textureMatrix[11] = 0;
    m_textureMatrix[12] = 0;
    m_textureMatrix[13] = 0;
    m_textureMatrix[14] = 0;
    m_textureMatrix[15] = 1;
}


Vector2 GFont::texelSize() const {
    return Vector2(charWidth, charHeight);
}


Vector2 GFont::drawString(
    RenderDevice*       renderDevice,
    const std::string&  s,
    float               x,
    float               y,
    float               w,
    float               h,
    Spacing             spacing) const {
    (void)renderDevice;

    debugAssert(renderDevice != NULL);
    const float propW = w / charWidth;
    const int n = s.length();

    // Shrink the vertical texture coordinates by 1 texel to avoid
    // bilinear interpolation interactions with mipmapping.
    float sy = h / charHeight;

    float x0 = 0;
    for (int i = 0; i < n; ++i) {
        unsigned char c = s[i] & (charsetSize - 1); // s[i] % charsetSize; avoid using illegal chars

        if (c != ' ') {
            int row   = c / 16;
            int col   = c & 15; // c % 16

            // Fixed width
            float sx = 0;
            
            if (spacing == PROPORTIONAL_SPACING) {
                sx = (charWidth - subWidth[(int)c]) * propW * 0.5f;
            }

            float xx = x - sx;
            glTexCoord2f(col * charWidth, row * charHeight + 1);
            glVertex2f(xx,     y + sy);

            glTexCoord2f(col * charWidth, (row + 1) * charHeight - 2);
            glVertex2f(xx,     y + h - sy); 

            xx += w;
            glTexCoord2f((col + 1) * charWidth - 1, (row + 1) * charHeight - 2);
            glVertex2f(xx, y + h - sy); 

            glTexCoord2f((col + 1) * charWidth - 1, row * charHeight + 1);
            glVertex2f(xx, y + sy);
                        
        }

        if (spacing == PROPORTIONAL_SPACING) {
            x += propW * subWidth[(int)c];
        } else {
            x += propW * subWidth[(int)'M'] * 0.85f;
        }
    }

    // TODO: update the RenderDevice vertex count
    return Vector2(x - x0, h);
}


Vector2 GFont::computePackedArray(
    const std::string&  s,
    float              x,
    float              y,
    float              w,
    float              h,
    Spacing             spacing,
    Array<Vector2>&     array) const {

    const float propW = w / charWidth;
    const int n = s.length();

    // Shrink the vertical texture coordinates by 1 texel to avoid
    // bilinear interpolation interactions with mipmapping.
    float sy = h / charHeight;

    float x0 = x;

    const float mwidth = subWidth[(int)'M'] * 0.85f * propW;

    int count = -1;
    for (int i = 0; i < n; ++i) {
        unsigned char c = s[i] & (charsetSize - 1); // s[i] % charsetSize; avoid using illegal chars

        if (c != ' ') {
            int row   = c >> 4; // fast version of c / 16
            int col   = c & 15; // fast version of c % 16

            // Fixed width
            float sx = (spacing == PROPORTIONAL_SPACING) ?
                (charWidth - subWidth[(int)c]) * propW * 0.5f : 0.0f;

            float xx = x - sx;
            // Tex, Vert
            ++count;
            array[count].x = col * charWidth;
            array[count].y = row * charHeight + 1;

            ++count;
            array[count].x = xx;
            array[count].y = y + sy;

            ++count;
            array[count].x = col * charWidth;
            array[count].y = (row + 1) * charHeight - 2;
            
            ++count;
            array[count].x = xx;
            array[count].y = y + h - sy;

            xx += w;
            ++count;
            array[count].x = (col + 1) * charWidth - 1;
            array[count].y = (row + 1) * charHeight - 2;
            
            ++count;
            array[count].x = xx;
            array[count].y = y + h - sy;
    
            ++count;
            array[count].x = (col + 1) * charWidth - 1;
            array[count].y = row * charHeight + 1;

            ++count;
            array[count].x = xx;
            array[count].y = y + sy;
        }

        x += (spacing == PROPORTIONAL_SPACING) ?
            propW * subWidth[(int)c] : mwidth;
    }

    return Vector2(x - x0, h);
}


void GFont::begin2DQuads(RenderDevice* renderDevice) const {

    renderDevice->setTextureMatrix(0, m_textureMatrix);
    renderDevice->setTexture(0, m_texture);
    // Apply bias to sharpen the MIP mapping
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -0.6f);
    debugAssertGLOk();
    
    renderDevice->setTextureCombineMode(0, RenderDevice::TEX_MODULATE);
        
    // This is BLEND_SRC_ALPHA because the texture has no luminance, only alpha
    renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA,
                               RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

    renderDevice->setAlphaTest(RenderDevice::ALPHA_GEQUAL, 1.0f / 255.0f);

    renderDevice->beforePrimitive();
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
}


void GFont::end2DQuads(RenderDevice* renderDevice) const {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);    
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0.0f);

    renderDevice->afterPrimitive();
}


// Used for vertex array storage
static Array<Vector2> array;


Vector2 GFont::send2DQuadsWordWrap(
    RenderDevice*               renderDevice,
    float                       maxWidth,
    const std::string&          s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    if (maxWidth == finf()) {
        return send2DQuads(renderDevice, s, pos2D, size, color, border, xalign, yalign, spacing);
    }

    Vector2 bounds = Vector2::zero();
    Point2 p = pos2D;
    std::string rest = s;
    std::string first = "";

    while (! rest.empty()) {
        wordWrapCut(maxWidth, rest, first, size, spacing);
        Vector2 extent = send2DQuads(renderDevice, first, p, size, color, border, xalign, yalign, spacing);
        bounds.x = max(bounds.x, extent.x);
        bounds.y += extent.y;
        p.y += iCeil(extent.y * 0.8f);
    }

    return bounds;
}

Vector2 GFont::send2DQuads(
    RenderDevice*               renderDevice,
    const std::string&          s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    float x = pos2D.x;
    float y = pos2D.y;

    float h = size * 1.5f;
    float w = h * charWidth / charHeight;

    int numChars = 0;
    for (unsigned int i = 0; i < s.length(); ++i) {
        // Spaces don't count as characters
        numChars += ((s[i] & (charsetSize - 1)) != ' ') ? 1 : 0;
    }

    if (numChars == 0) {
        return Vector2(0, h);
    }

    switch (xalign) {
    case XALIGN_RIGHT:
        x -= bounds(s, size, spacing).x;
        break;

    case XALIGN_CENTER:
        x -= bounds(s, size, spacing).x / 2;
        break;
    
    default:
        break;
    }

    switch (yalign) {
    case YALIGN_CENTER:
        y -= h / 2.0;
        break;

    case YALIGN_BASELINE:
        y -= baseline * h / (float)charHeight;
        break;

    case YALIGN_BOTTOM:
        y -= h;
        break;

    default:
        break;
    }

    // Packed vertex array; tex coord and vertex are interlaced
    // For each character we need 4 vertices.
    
    array.resize(numChars * 4 * 2, DONT_SHRINK_UNDERLYING_ARRAY);
    const Vector2 bounds = computePackedArray(s, x, y, w, h, spacing, array);
    
    int N = numChars * 4;
    

    // 2 coordinates per element, float elements, stride (for interlacing), count, pointer
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2) * 2, &array[0]);
    glVertexPointer(2, GL_FLOAT, sizeof(Vector2) * 2, &array[1]);
    
    if (border.a > 0.05f) {
        renderDevice->setColor(border);
        glMatrixMode(GL_MODELVIEW);
        float lastDx = 0, lastDy = 0;
        for (int dy = -1; dy <= 1; dy += 2) {
            for (int dx = -1; dx <= 1; dx += 2) {
                if ((dx != 0) || (dy != 0)) {
                    // Shift modelview matrix by dx, dy, but also undo the 
                    // shift from the previous outline
                    glTranslatef(dx - lastDx, dy - lastDy, 0);
                    glDrawArrays(GL_QUADS, 0, N);
                    lastDx = dx; lastDy = dy;
                }
            }
        }
        glTranslatef(-lastDx, -lastDy, 0);
    }

    // Draw foreground
    renderDevice->setColor(color);
    glDrawArrays(GL_QUADS, 0, N);
    debugAssertGLOk();

    return bounds;
}

Vector2 GFont::draw2D(
    RenderDevice*               renderDevice,
    const std::string&          s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    debugAssert(renderDevice != NULL);

    Vector2 bounds;
    renderDevice->pushState();
        renderDevice->disableLighting();

        begin2DQuads(renderDevice);
        bounds = send2DQuads(renderDevice, s, pos2D, size, color, border, xalign, yalign, spacing);
        end2DQuads(renderDevice);
    renderDevice->popState();
    debugAssertGLOk();

    return bounds;
}


Vector2 GFont::draw3DBillboard(
    RenderDevice*               renderDevice,
    const std::string&          s,
    const Vector3&              pos3D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    CFrame cframe = pos3D;
    const CFrame& camera = renderDevice->cameraToWorldMatrix();
    const CFrame& object = renderDevice->objectToWorldMatrix();
    
    cframe.rotation = camera.rotation * object.rotation.transpose();
    return draw3D(renderDevice, s, cframe, size, color, border, xalign, yalign, spacing);
}


Vector2 GFont::draw3D(
    RenderDevice*               renderDevice,
    const std::string&          s,
    const CoordinateFrame&      pos3D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {
    
    debugAssert(renderDevice != NULL);

    float x = 0;
    float y = 0;

    float h = size * 1.5;
    float w = h * charWidth / charHeight;

    switch (xalign) {
    case XALIGN_RIGHT:
        x -= bounds(s, size, spacing).x;
        break;

    case XALIGN_CENTER:
        x -= bounds(s, size, spacing).x / 2;
        break;
    
    default:
        break;
    }

    switch (yalign) {
    case YALIGN_CENTER:
        y -= h / 2.0;
        break;

    case YALIGN_BASELINE:
        y -= baseline * h / (float)charHeight;
        break;

    case YALIGN_BOTTOM:
        y -= h;
        break;

    default:
        break;
    }

    Vector2 bounds;
    renderDevice->pushState();

        // We need to get out of Y=up coordinates.
        CoordinateFrame flipY;
        flipY.rotation[1][1] = -1;
        renderDevice->setObjectToWorldMatrix(pos3D * flipY);

        renderDevice->setTextureMatrix(0, m_textureMatrix);
        renderDevice->setTexture(0, m_texture);
        
        renderDevice->setTextureCombineMode(0, RenderDevice::TEX_MODULATE);
            
        // This is BLEND_SRC_ALPHA because the texture has no luminance, only alpha
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA,
                                   RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        renderDevice->setAlphaTest(RenderDevice::ALPHA_GEQUAL, 1.0f / 255.0f);
        renderDevice->setCullFace(RenderDevice::CULL_NONE);
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);

        renderDevice->disableLighting();
        bool d = renderDevice->depthWrite();
        if (border.a > 0.05f) {

            renderDevice->setDepthWrite(false);
            renderDevice->beginPrimitive(PrimitiveType::QUADS);
            // Make the equivalent of a 3D "1 pixel" offset (the
            // default 2D text size is 12-pt with a 1pix border)

            const float borderOffset = size / 12.0;
            renderDevice->setColor(border);
            for (int dy = -1; dy <= 1; dy += 2) {
                for (int dx = -1; dx <= 1; dx += 2) {
                    if ((dx != 0) || (dy != 0)) {
                        drawString(renderDevice, s,
	                       x + dx * borderOffset, 
	                       y + dy * borderOffset,
	                       w, h, spacing);
                    }
                }
            }
            renderDevice->endPrimitive();
            renderDevice->setDepthWrite(d);
        }

        // Draw the main body
        renderDevice->setColor(color);
        renderDevice->beginPrimitive(PrimitiveType::QUADS);
        bounds = drawString(renderDevice, s, x, y, w, h, spacing);
        renderDevice->endPrimitive();

    renderDevice->popState();

    return bounds;
}
    
Vector2 GFont::draw2DWordWrap(
    RenderDevice*               renderDevice,
    float                       maxWidth,
    const std::string&          s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    debugAssert(renderDevice != NULL);

    Vector2 bounds;
    renderDevice->pushState();
    {
        renderDevice->disableLighting();

        begin2DQuads(renderDevice);

        bounds = send2DQuadsWordWrap(renderDevice, maxWidth, s, pos2D, size, color, border, xalign, yalign, spacing);

        end2DQuads(renderDevice);
    }
    renderDevice->popState();
    debugAssertGLOk();

    return bounds;
}

    
int GFont::wordWrapCut(
    float               maxWidth,
    std::string&        s,
    std::string&        firstPart,
    float               size,
    Spacing             spacing) const {
    debugAssert(maxWidth > 0);
    int n = s.length();

    const float h = size * 1.5;
    const float w = h * charWidth / charHeight;
    const float propW = w / charWidth;
    float x = 0;

    // Current character number
    int i = 0;

    // Walk forward until we hit the end of the string or the maximum width
    while ((x <= maxWidth) && (i < n)) {
        const unsigned char c = s[i] & (charsetSize - 1);
        if (spacing == PROPORTIONAL_SPACING) {
            x += propW * subWidth[(int)c];
        } else {
            x += propW * subWidth[(int)'M'] * 0.85;
        }
        ++i;
    }

    if (i == n) {
        // We made it to the end
        firstPart = s;
        s = "";
        return n;
    }

    --i;

    // Search backwards for a space.  Only look up to 25% of maxWidth
    while ((i > 1) && ! isWhiteSpace(s[i]) && (x > maxWidth * 0.25f)) {
        const unsigned char c = s[i] & (charsetSize - 1);
        if (spacing == PROPORTIONAL_SPACING) {
            x -= propW * subWidth[(int)c];
        } else {
            x -= propW * subWidth[(int)'M'] * 0.85;
        }
        --i;
    }

    firstPart = s.substr(0, i);
    s = trimWhitespace(s.substr(i));

    return i;
}


Vector2 GFont::bounds(
    const std::string&  s,
    float               size,
    Spacing             spacing) const {

    int n = s.length();

    float h = size * 1.5;
    float w = h * charWidth / charHeight;
    float propW = w / charWidth;
    float x = 0;
    float y = h;

    if (spacing == PROPORTIONAL_SPACING) {
        for (int i = 0; i < n; ++i) {
            unsigned char c = s[i] & (charsetSize - 1);
            x += propW * subWidth[(int)c];
        }
    } else {
        x = subWidth[(int)'M'] * n * 0.85 * propW;
    }

    return Vector2(x, y);
}


void GFont::makeFont(int charsetSize, const std::string& infileBase, std::string outfile) {

    debugAssert(FileSystem::exists(infileBase + ".tga"));
    debugAssert(FileSystem::exists(infileBase + ".ini"));
    debugAssert(charsetSize == 128 || charsetSize == 256);

    if (outfile == "") {
        outfile = infileBase + ".fnt";
    }

    GImage image(infileBase + ".tga");
    debugAssert(isPow2(image.width()));
    debugAssert(isPow2(image.height()));
    image.convertToL8();

    TextInput    ini(infileBase + ".ini");
    BinaryOutput out(outfile, G3D_LITTLE_ENDIAN);

    ini.readSymbol("[");
    ini.readSymbol("Char");
    ini.readSymbol("Widths");
    ini.readSymbol("]");

    // Version
    out.writeInt32(2);

    // Number of characters
    out.writeInt32(charsetSize);

    // Character widths
    for (int i = 0; i < charsetSize; ++i) {
        int n = (int)ini.readNumber();
        (void)n;
        debugAssert(n == i);
        ini.readSymbol("=");
        int cw = (int)ini.readNumber();
        out.writeInt16(cw);
    }
    
    // Autodetect baseline from capital E
    {
        // Size of a character, in texels
        int          w        = image.width() / 16;

        int          x0       = ('E' % 16) * w;
        int          y0       = ('E' / 16) * w;
        
        const uint8* p        = image.byte();
        bool         done     = false;
        int          baseline = w * 2 / 3;
    
        // Search up from the bottom for the first pixel
        for (int y = y0 + w - 1; (y >= y0) && ! done; --y) {
            for (int x = x0; (x < x0 + w) && ! done; ++x) {
                if (p[x + y * w * 16] != 0) {
                    baseline = y - y0 + 1;
                    done = true;
                }
            }
        }
        out.writeUInt16(baseline);
    }

    // Texture width
    out.writeUInt16(image.width());
    out.writeBytes(image.byte(), square(image.width()) * 256 / charsetSize);
 
    out.compress();
    out.commit(false);
}

}
