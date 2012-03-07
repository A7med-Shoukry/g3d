/**
  \file GImage.cpp
  \author Morgan McGuire, http://graphics.cs.williams.edu
  Copyright 2002-2011, Morgan McGuire

  \created 2002-05-27
  \edited  2011-06-23
 */
#include "G3D/platform.h"
#include "G3D/GImage.h"
#include "G3D/debug.h"
#include "G3D/stringutils.h"
#include "G3D/TextInput.h"
#include "G3D/TextOutput.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Log.h"
#include "G3D/fileutils.h"

#ifdef G3D_LINUX
#    include <png.h>
#else
#    include "png.h"
#endif

#include <sys/stat.h>
#include <assert.h>
#include <sys/types.h>

//////////////////////////////////////////////////////////////////////////////////////////////

namespace G3D {

void GImage::LtoRGBA(
    const unorm8*   in,
    unorm8*          out,
    int             numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        unorm8 v = in[i];
        int i4 = i * 4;

        out[i4 + 0] = v;
        out[i4 + 1] = v;
        out[i4 + 2] = v;
        out[i4 + 3] = unorm8::fromBits(255);
    }
}


void GImage::LtoRGB(
    const unorm8*    in,
    unorm8*          out,
    int             numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        const unorm8 v = in[i];
        int i3 = i * 3;

        out[i3 + 0] = v;
        out[i3 + 1] = v;
        out[i3 + 2] = v;
    }
}

   
void GImage::RGBtoRGBA(
    const unorm8*    in,
    unorm8*          out,
    int             numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        int i3 = i * 3;
        int i4 = i3 + i;

        out[i4 + 0] = in[i3 + 0]; 
        out[i4 + 1] = in[i3 + 1]; 
        out[i4 + 2] = in[i3 + 2]; 
        out[i4 + 3] = unorm8::one();
    }
}


void GImage::RGBAtoRGB(
    const unorm8*    in,
    unorm8*          out,
    int              numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        int i3 = i * 3;
        int i4 = i3 + i;

        out[i3 + 0] = in[i4 + 0]; 
        out[i3 + 1] = in[i4 + 1]; 
        out[i3 + 2] = in[i4 + 2]; 
    }
}


void GImage::RGBtoBGRA(
    const unorm8*    in,
    unorm8*          out,
    int                     numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        int i3 = i * 3;
        int i4 = i3 + i;

        out[i4 + 2] = in[i3 + 0]; 
        out[i4 + 1] = in[i3 + 1]; 
        out[i4 + 0] = in[i3 + 2]; 
        out[i4 + 3] = unorm8::one(); 
    }
}


void GImage::RGBAtoBGRA
(const unorm8*    in,
 unorm8*          out,
 int              numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        int i4 = i * 4;

        out[i4 + 0] = in[i4 + 2]; 
        out[i4 + 1] = in[i4 + 1]; 
        out[i4 + 2] = in[i4 + 0]; 
        out[i4 + 3] = in[i4 + 3]; 
    }
}
    


void GImage::RGBtoBGR
(const unorm8*    in,
 unorm8*          out,
 int              numPixels) {
    
    for (int i = 0; i < numPixels; ++i) {
        const int i3 = i * 3;

        const unorm8 r = in[i3 + 0];
        const unorm8 g = in[i3 + 1];
        const unorm8 b = in[i3 + 2];

        out[i3 + 2] = r; 
        out[i3 + 1] = g; 
        out[i3 + 0] = b;
    }
}


void GImage::RGBxRGBtoRGBA
(const unorm8*            colorRGB,
 const unorm8*            alphaRGB,
 unorm8*                  out,
 int                      numPixels) {

    for (int i = numPixels - 1; i >= 0; --i) {
        const int i3 = i * 3;
        const int i4 = i3 + i;

        out[i4 + 0] = colorRGB[i3 + 0];
        out[i4 + 1] = colorRGB[i3 + 1];
        out[i4 + 2] = colorRGB[i3 + 2];
        out[i4 + 3] = alphaRGB[i3 + 0];
    }
}


void GImage::RGBtoARGB
(const unorm8*            in,
 unorm8*                  out,
 int                      numPixels) {

    for (int i = 0; i < numPixels; ++i) {
        const int i3 = i * 3;
        const int i4 = i3 + i;

        out[i4 + 0] = unorm8::fromBits(255);
        out[i4 + 1] = in[i3 + 0]; 
        out[i4 + 2] = in[i3 + 1]; 
        out[i4 + 3] = in[i3 + 2];
    }
}


void GImage::flipRGBVertical
(const unorm8*           in,
 unorm8*                 out,
 int                     width,
 int                     height) {

    
    // Allocate a temp row so the operation
    // is still safe if in == out
    unorm8* temp = (unorm8*)System::malloc(width * 3);
    alwaysAssertM(temp != NULL, "Out of memory"); 

    int oneRow = width * 3;
    int N = height / 2;

    // if height is an odd value, don't swap odd middle row
    for (int i = 0; i < N; ++i) {
        int topOff = i * oneRow;
        int botOff = (height - i - 1) * oneRow;
        System::memcpy(temp,         in + topOff, oneRow);
        System::memcpy(out + topOff, in + botOff, oneRow);
        System::memcpy(out + botOff, temp,        oneRow);
    }

    System::free(temp);
}


void GImage::flipRGBAVertical(
    const unorm8*            in,
    unorm8*                  out,
    int                     width,
    int                     height) {

    
    // Allocate a temp row so the operation
    // is still safe if in == out
    unorm8* temp = (unorm8*)System::malloc(width * 4);
    alwaysAssertM(temp != NULL, "Out of memory");

    int oneRow = width * 4;

    // if height is an odd value, don't swap odd middle row
    for (int i = 0; i < height / 2; ++i) {
        int topOff = i * oneRow;
        int botOff = (height - i - 1) * oneRow;
        System::memcpy(temp,         in + topOff, oneRow);
        System::memcpy(out + topOff, in + botOff, oneRow);
        System::memcpy(out + botOff, temp,        oneRow);
    }

    System::free(temp);
}

////////////////////////////////////////////////////////////////////////////////////////

void GImage::decode(
    BinaryInput&        input,
    Format              format) {

    switch (format) {
    case PPM_ASCII:
        decodePPMASCII(input);
        break;

    case PPM_BINARY:
        decodePPM(input);
        break;

    case PNG:
        decodePNG(input);
        break;

    case JPEG:
        decodeJPEG(input);
        break;

    case TGA:
        decodeTGA(input);
        break;

    case BMP:
        decodeBMP(input);
        break;

    case ICO:
        decodeICO(input);
        break;

    case PCX:
        decodePCX(input);
        break;

    default:
        debugAssert(false);
    }

    debugAssert(width() >= 0);
    debugAssert(height() >= 0);
    debugAssert(channels() == 1 || channels() == 3 || channels() == 4);
    debugAssert(m_buffer.notNull());
}



void GImage::decodePCX
(BinaryInput&                input) {

    uint8  manufacturer = input.readUInt8();
    uint8  version      = input.readUInt8();
    uint8  encoding     = input.readUInt8();
    uint8  bitsPerPixel = input.readUInt8();

    uint16 xmin         = input.readUInt16();
    uint16 ymin         = input.readUInt16();
    uint16 xmax         = input.readUInt16();
    uint16 ymax         = input.readUInt16();

    uint16 horizDPI     = input.readUInt16();
    uint16 vertDPI      = input.readUInt16();

    Color3unorm8 colorMap[16];
    input.readBytes(colorMap, 48);

    input.skip(1);

    uint8  planes       = input.readUInt8();
    uint16 bytesPerLine = input.readUInt16();
    uint16 paletteType  = input.readUInt16();
    input.skip(4 + 54);

    (void)bytesPerLine;

    int width  = xmax - xmin + 1;
    int height = ymax - ymin + 1;

    if ((manufacturer != 0x0A) || (encoding != 0x01)) {
        throw GImage::Error("PCX file is corrupted", input.getFilename());
    }

    (void)version;
    (void)vertDPI;
    (void)horizDPI;

    if ((bitsPerPixel != 8) || ((planes != 1) && (planes != 3))) {
        throw GImage::Error("Only 8-bit paletted and 24-bit PCX files supported.", input.getFilename());
    }

    // Prepare the pointer object for the pixel data
    m_buffer = ImageBuffer::create(m_memMan, ImageFormat::RGB8(), width, height);

    if ((paletteType == 1) && (planes == 3)) {

        Color3unorm8* pixel = pixel3();

        // Iterate over each scan line
        for (int row = 0; row < height; ++row) {
            // Read each scan line once per plane
            for (int plane = 0; plane < planes; ++plane) {
                int p = row * width;
                int p1 = p + width;
                while (p < p1) {
                    uint8 value = input.readUInt8();
                    int length = 1;
            
                    if (value >= 192) {
                        // This is the length, not the value.  Mask off
                        // the two high bits and read the true index.
                        length = value & 0x3F;
                        value = input.readUInt8();
                    }

                    // Set the whole run
                    for (int i = length - 1; i >= 0; --i, ++p) {
                        debugAssert(p < width * height);
                        pixel[p][plane] = unorm8::fromBits(value);
                    }
                }
            }
        }

    } else if (planes == 1) {

        Color3unorm8 palette[256];

        int imageBeginning   = input.getPosition();
        int paletteBeginning = input.getLength() - 769;

        input.setPosition(paletteBeginning);

        uint8 dummy = input.readUInt8();

        if (dummy != 12) {
            Log::common()->println("\n*********************");
            Log::common()->printf("Warning: Corrupted PCX file (palette marker byte was missing) \"%s\"\nLoading anyway\n\n", input.getFilename().c_str());
        }

        input.readBytes(palette, sizeof(palette));
        input.setPosition(imageBeginning);
        
        Color3unorm8* pixel = pixel3();
        
        // The palette indices are run length encoded.
        int p = 0;
        while (p < width * height) {
            uint8 index  = input.readUInt8();
            uint8 length = 1;

            if (index >= 192) {
                // This is the length, not the index.  Mask off
                // the two high bits and read the true index.
                length = index & 0x3F;
                index  = input.readUInt8();
            }

            Color3unorm8 color = palette[index];

            // Set the whole run
            for (int i = length - 1; i >= 0; --i, ++p) {
                if (p > width * height) {
                    break;
                }
                pixel[p] = color;
            }

        }

    } else {
        throw GImage::Error("Unsupported PCX file type.", input.getFilename());
    }
}


GImage::Format GImage::resolveFormat(const std::string&  filename) {
    BinaryInput b(filename, G3D_LITTLE_ENDIAN);
    if (b.size() <= 0) {
        throw Error("File not found.", filename);
    }

    return resolveFormat(filename, b.getCArray(), b.size(), AUTODETECT);
}


GImage::Format GImage::resolveFormat
(const std::string&  filename,
 const uint8*        data,
 int                 dataLen,
 Format              maybeFormat) {
    
    // Return the provided format if it is specified.
    if (maybeFormat != AUTODETECT) {
        return maybeFormat;
    }

    std::string extension = toUpper(filenameExt(filename));

    if ((extension == "PPM") || (extension == "PGM") || (extension == "PBM")) {
        // There are two PPM formats (binary and ASCII); we handle them differently
        if (dataLen > 3) {
            if (!memcmp(data, "P6", 2) || !memcmp(data, "P5", 2)) {
                return PPM_BINARY;
            } else {
                return PPM_ASCII;
            }
        }
    }

    Format tmp = stringToFormat(extension);
    if ((tmp != AUTODETECT) && (tmp != UNKNOWN)) {
        return tmp;
    }

    // Try and autodetect from the file itself by looking at the first
    // character.

    // We can't look at the character if it is null.
    debugAssert(data != NULL);              

    if ((dataLen > 3) && (! memcmp(data, "P3", 2) || (! memcmp(data, "P2", 2)) || (! memcmp(data, "P1", 2)))) {
        return PPM_ASCII;
    }

    if ((dataLen > 3) && (!memcmp(data, "P6", 2) ||!memcmp(data, "P5", 2))) {
        return PPM_BINARY;
    }

    if (dataLen > 8) {
        if (!png_sig_cmp((png_bytep)data, 0, 8)) {
            return PNG;
        }
    }

    if ((dataLen > 0) && (data[0] == 'B')) {
        return BMP;
    }

    if (dataLen > 10) {
        if ((dataLen > 11) && (data[0] == 0xFF) &&
            (memcmp(&data[6], "JFIF", 4) == 0)) {
            return JPEG;
        }
    }

    if (dataLen > 40) {
        if (memcmp(&data[dataLen - 18], "TRUEVISION-XFILE", 16) == 0) {
            return TGA;
        }
    }

    if ((dataLen > 4) && (data[0] == 0) && (data[1] == 0) && (data[2] == 0) && (data[3] == 1)) {
        return ICO;
    }

    if ((dataLen > 0) && (data[0] == 10)) {
        return PCX;
    }

    return UNKNOWN;
}


GImage::GImage
   (const std::string&  filename,
    Format              format,
    const MemoryManager::Ref& m) : 
    m_memMan(m) {
    
    load(filename, format);
}


void GImage::load
   (const std::string&  filename,
    Format              format) {

    clear();

    try {
        BinaryInput b(filename, G3D_LITTLE_ENDIAN);
        if (b.size() <= 0) {
            throw Error("File not found.", filename);
        }

        alwaysAssertM(this != NULL, "Corrupt GImage");
        decode(b, resolveFormat(filename, b.getCArray(), b.size(), format));
    } catch (const std::string& error) {
        throw Error(error, filename);
    }
}


GImage::GImage
(const uint8*        data,
 int                 length,
 Format              format,
    const MemoryManager::Ref& m) : 
    m_memMan(m) {

    BinaryInput b(data, length, G3D_LITTLE_ENDIAN);
    // It is safe to cast away the const because we
    // know we don't corrupt the data.

    decode(b, resolveFormat("", data, length, format));
}


GImage::GImage
   (int                 width,
    int                 height,
    int                 channels,
    const MemoryManager::Ref& mem) : 
    m_memMan(mem) {
    
    resize(width, height, channels);
}


GImage::GImage
   (int                 width,
    int                 height,
    const ImageFormat*  im,
    const MemoryManager::Ref& mem) : 
    m_memMan(mem) {
}


void GImage::resize
   (int                 width,
    int                 height,
    int                 channels,
    bool                zero) {

    const ImageFormat* im = ImageFormat::RGB8();

    switch (channels) {
    case 1:
        im = ImageFormat::R8();
        break;
    case 3:
        im = ImageFormat::RGB8();
        break;
    case 4:
        im = ImageFormat::RGBA8();
        break;
    default:
        alwaysAssertM(false, "Illegal number of channels.");
    }
    resize(width, height, im, zero);
}


void GImage::resize
   (int                 width,
    int                 height,
    const ImageFormat*  im,
    bool                zero) {

    debugAssert(width >= 0);
    debugAssert(height >= 0);
    debugAssert(im != NULL);

    if ((this->width() == width) && (this->height() == height) && (imageFormat() == im) && (zero == false)) {
        // Nothing to do
        return;
    }

    clear();

    m_buffer = ImageBuffer::create(m_memMan, im, width, height);

    if (zero) {
        System::memset(m_buffer->buffer(), 0, m_buffer->size());
    }
}


void GImage::_copy
   (const GImage&       other) {

    clear();

    // DISABLED FOR ImageBuffer
    //m_buffer = other.buffer().copy();
}


void GImage::flipHorizontal() {
    unorm8 temp[4];
    int rowBytes = m_buffer->stride();
    (void)rowBytes;
    for (int y = 0; y < height(); ++y) {
        uint8* row = static_cast<uint8*>(m_buffer->row(y));
        for (int x = 0; x < width() / 2; ++x) { 
            System::memcpy(temp, row + x * channels(), channels());
            System::memcpy(row + x * channels(), row + (width() - x - 1) * channels(), channels());
            System::memcpy(row + (width() - x - 1) * channels(), temp, channels());
        }
    }
}


void GImage::flipVertical() {
    ImageBuffer::Ref old = m_buffer;
    m_buffer = ImageBuffer::create(m_memMan, imageFormat(), width(), height());

    // We could do this with only a single-row temp buffer, but then
    // we'd have to copy twice as much data.
    for (int y = 0; y < height(); ++y) {
        System::memcpy(m_buffer->row(y), old->row(height() - y - 1), old->stride());
    }
}


void GImage::rotate90CW(int numTimes) {
    /* DISABLED FOR CONVERSION TO ImageBuffer
    uint8* old = NULL;
    numTimes = iWrap(numTimes, 4);
    if (numTimes > 0) {
        old = (uint8*)m_memMan->alloc(m_width * m_height * m_channels);
    }
    for (int j = 0; j < numTimes; ++j) {
        {
            uint8* temp = old;
            old = m_byte;
            m_byte = temp;
        }
        
        {
            int temp = m_width;
            m_width = m_height;
            m_height = temp;
        }
        
        int rowBytes = m_width * m_channels;
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                uint8* dst = m_byte + x + y * rowBytes;
                uint8* src = old + y + (m_height - x - 1) * rowBytes;
                System::memcpy(dst, src, m_channels);
            }
        }
    }
    */
}



GImage::GImage(
    const GImage&        other,
    const MemoryManager::Ref& m) : m_memMan(m) {

    _copy(other);
}


GImage::~GImage() {
    clear();
}


void GImage::clear() {
    m_buffer = NULL;
}


GImage& GImage::operator=(const GImage& other) {
    _copy(other);
    return *this;
}


bool GImage::copyRect(
    GImage & dest, const GImage & src,
    int srcX, int srcY, int srcWidth, int srcHeight) {
    if ((src.width() < srcX + srcWidth) ||
        (src.height() < srcY + srcHeight) ||
        (srcY < 0) ||
        (srcX < 0)) {

        return false;
    }

    dest.resize(srcWidth, srcHeight, src.channels());
    
    bool ret;
    ret = copyRect(dest, src, 0, 0, srcX, srcY, srcWidth, srcHeight);
    debugAssert(ret);

    return true;
}


bool GImage::copyRect
(GImage& dest,
 const GImage& src,
 int destX, 
 int destY,
 int srcX,
 int srcY, 
 int srcWidth, 
 int srcHeight) {
    
    if ((src.width() < srcX + srcWidth) ||
        (src.height() < srcY + srcHeight) ||
        (dest.width() < destX + srcWidth) ||
        (dest.height() < destY + srcHeight) ||
        (srcY < 0) ||
        (srcX < 0) ||
        (destY < 0) ||
        (destX < 0) ||
        (src.channels() != dest.channels())) {

        return false;
    }

    for (int i = 0; i < srcHeight; i++) {
        const uint8* srcRow = src.byte() +
            ((i + srcY) * src.width() + srcX) * src.channels();
        uint8* destRow = dest.byte() +
            ((i + destY) * dest.width() + destX) * dest.channels();
        memcpy(destRow, srcRow, srcWidth * src.channels());
    }

    return true;
}


bool GImage::supportedFormat(
    const std::string& format) {

    return (stringToFormat(format) != UNKNOWN);
}


GImage::Format GImage::stringToFormat(
    const std::string& format) {

    std::string extension = toUpper(format);

    if ((extension == "JPG") || (extension == "JPEG")) {
        return JPEG;
    } else if (extension == "TGA") {
        return TGA;
    } else if (extension == "BMP") {
        return BMP;
    } else if (extension == "PCX") {
        return PCX;
    } else if (extension == "ICO") {
        return ICO;
    } else if (extension == "PNG") {
        return PNG;
    } else {
        return UNKNOWN;
    }
    // Don't put PPM here, since it has two versions
}


void GImage::save(
    const std::string& filename,
    Format             format) const {

    BinaryOutput b(filename, G3D_LITTLE_ENDIAN);
    encode(resolveFormat(filename, NULL, 0, format), b);
    b.commit(false);
}


void GImage::encode(
    Format              format,
    unorm8*&             outData,
    int&                outLength) const {

    BinaryOutput out;

    encode(format, out);

    outData = (unorm8*)System::malloc(out.size());
    debugAssert(outData);
    outLength = out.size();

    out.commit((uint8*)outData);
}


void GImage::encode(
    Format              format,
    BinaryOutput&       out) const {

    switch (format) {
    case PPM_ASCII:
        encodePPMASCII(out);
        break;

    case PPM_BINARY:
        encodePPM(out);
        break;

    case PNG:
        encodePNG(out);
        break;

    case JPEG:
        encodeJPEG(out);
        break;

    case BMP:
        encodeBMP(out);
        break;

    case TGA:
        encodeTGA(out);
        break;

    default:
        debugAssert(false);
    }
}


void GImage::insertRedAsAlpha(const GImage& alpha, GImage& output) const {
    debugAssert(alpha.width() == width());
    debugAssert(alpha.height() == height());

    // make sure output GImage is valid
    if (output.width() != width() || output.height() != height() || output.channels() != 4) {
        output.resize(width(), height(), 4);
    }

    int N = width() * height();
    for (int i = 0; i < N; ++i) {
        output.byte()[i * 4 + 0] = byte()[i * channels() + 0];
        output.byte()[i * 4 + 1] = byte()[i * channels() + 1];
        output.byte()[i * 4 + 2] = byte()[i * channels() + 2];
        output.byte()[i * 4 + 3] = alpha.byte()[i * alpha.channels()];
    }
}


void GImage::stripAlpha(GImage& output) const {

    if (output.width() != width() || output.height() != height() || output.channels() != 3) {
        output.resize(width(), height(), 3);
    }

    int N = width() * height();
    for (int i = 0; i < N; ++i) {
        output.byte()[i * 3 + 0] = byte()[i * channels() + 0];
        output.byte()[i * 3 + 1] = byte()[i * channels() + 1];
        output.byte()[i * 3 + 2] = byte()[i * channels() + 2];
    }
}


int GImage::sizeInMemory() const {
    return sizeof(GImage) + m_buffer.notNull() ? m_buffer->size() : 0;
}


void GImage::computeNormalMap(
    const GImage&       bump,
    GImage&             normal,
    const BumpMapPreprocess& preprocess) {
    computeNormalMap(bump.width(), bump.height(), bump.channels(), 
                     bump.rawData<unorm8>(), normal, preprocess);    
}


void GImage::computeNormalMap
(int                 width,
 int                 height,
 int                 channels,
 const unorm8*       src,
 GImage&             normal,
 const BumpMapPreprocess& preprocess) {
    
    float whiteHeightInPixels = preprocess.zExtentPixels;
    bool lowPassBump          = preprocess.lowPassFilter;
    bool scaleHeightByNz      = preprocess.scaleZByNz;
    
    if (whiteHeightInPixels < 0.0f) {
        // Default setting scales so that a gradient ramp
        // over the whole image becomes a 45-degree angle
        
        // Account for potentially non-square aspect ratios
        whiteHeightInPixels = max(width, height) * -whiteHeightInPixels;
    }

    debugAssert(whiteHeightInPixels >= 0);

    const int w = width;
    const int h = height;
    const int stride = channels;

    normal.resize(w, h, ImageFormat::RGBA8());

    const unorm8* const B = src;
    Color4unorm8* const N = normal.pixel4();

    // 1/s for the scale factor that each ELEVATION should be multiplied by.
    // We avoid actually multiplying by this and instead just divide it out of z.
    float elevationInvScale = 255.0f / whiteHeightInPixels;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Index into normal map pixel
            int i = x + y * w;

            // Index into bump map *byte*
            int j = stride * i;

            Vector3 delta;

            // Get a value from B (with wrapping lookup) relative to (x, y)
            // and divide by 255
#define ELEVATION(DX, DY)  ((int)(B[(((DX + x + w) % w) +               \
                                     ((DY + y + h) % h) * w) * stride].bits()))


            // Sobel filter to compute the normal.  
            //
            // Y Filter (X filter is the transpose)
            //  [ -1 -2 -1 ]
            //  [  0  0  0 ]
            //  [  1  2  1 ]

            // Write the Y value directly into the x-component so we don't have
            // to explicitly compute a cross product at the end.  Does not 
            // go out of bounds because the above is computed mod (width, height)
            delta.y = -( ELEVATION(-1, -1) * 1 +  ELEVATION( 0, -1) * 2 +  ELEVATION( 1, -1) * 1 +
                        -ELEVATION(-1,  1) * 1 + -ELEVATION( 0,  1) * 2 + -ELEVATION( 1,  1) * 1);

            delta.x = -(-ELEVATION(-1, -1) * 1 + ELEVATION( 1, -1) * 1 + 
                        -ELEVATION(-1,  0) * 2 + ELEVATION( 1,  0) * 2 + 
                        -ELEVATION(-1,  1) * 1 + ELEVATION( 1,  1) * 1);

            // The scale of each filter row is 4, the filter width is two pixels,
            // and the "normal" range is 0-255.
            delta.z = 4 * 2 * elevationInvScale;

            // Delta is now scaled in pixels; normalize 
            delta = delta.direction();

            // Copy over the bump value into the alpha channel.
            float H = B[j];

            if (lowPassBump) {
                H = (ELEVATION(-1, -1) + ELEVATION( 0, -1) + ELEVATION(1, -1) +
                     ELEVATION(-1,  0) + ELEVATION( 0,  0) + ELEVATION(1,  0) +
                     ELEVATION(-1,  1) + ELEVATION( 0,  1) + ELEVATION(1,  1)) / (255.0f * 9.0f);
            }
#           undef ELEVATION

            if (scaleHeightByNz) {
                // delta.z can't possibly be negative, so we avoid actually
                // computing the absolute value.
                H *= delta.z;
            }

            N[i].a = unorm8(H);

            // Pack into byte range
            delta = delta * 0.5f + Vector3(0.5f, 0.5f, 0.5f);
            N[i].r = unorm8(delta.x);
            N[i].g = unorm8(delta.y);
            N[i].b = unorm8(delta.z);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GImage::convertToL8() {
    switch (channels()) {
    case 1:
        return;

    case 3:
        {            
            // Average
            ImageBuffer::Ref old = m_buffer;
            Color3unorm8* src = pixel3();
            resize(width(), height(), 1);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const Color3unorm8   s = src[i];
                uint8&               d = static_cast<uint8*>(m_buffer->buffer())[i]; 
                d = ((int)s.r.bits() + (int)s.g.bits() + (int)s.b.bits()) / 3;
            }
        }
        break;

    case 4:
        {            
            // Average
            ImageBuffer::Ref old = m_buffer;
            Color4unorm8* src = pixel4();
            resize(width(), height(), 1);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const Color4unorm8   s = src[i];
                uint8&               d = static_cast<uint8*>(m_buffer->buffer())[i]; 
                d = ((int)s.r.bits() + (int)s.g.bits() + (int)s.b.bits()) / 3;
            }
        }
        return;

    default:
        alwaysAssertM(false, "Bad number of channels in input image");
    }
}


void GImage::convertToRGBA() {
    switch (channels()) {
    case 1:
        {            
            // Spread
            ImageBuffer::Ref old = m_buffer;
            resize(width(), height(), 4);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const unorm8  s = unorm8::fromBits(static_cast<uint8*>(old->buffer())[i]);
                Color4unorm8& d = pixel4()[i]; 
                d.r = d.g = d.b = s;
                d.a = unorm8::one();
            }
        }
        break;

    case 3:
        {            
            // Add alpha
            ImageBuffer::Ref old = m_buffer;
            Color3unorm8* src = pixel3();
            resize(width(), height(), 4);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const Color3unorm8   s = src[i];
                Color4unorm8&        d = pixel4()[i]; 
                d.r = s.r;
                d.g = s.g;
                d.b = s.b;
                d.a = unorm8::one();
            }
        }
        break;

    case 4:
        // Already RGBA
        return;

    default:
        alwaysAssertM(false, "Bad number of channels in input image");
    }
}


void GImage::convertToRGB() {
    switch (channels()) {
    case 1:
        {            
            // Spread
            ImageBuffer::Ref old = m_buffer;
            resize(width(), height(), 3);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const unorm8  s = static_cast<unorm8*>(old->buffer())[i];
                Color3unorm8& d = pixel3()[i]; 
                d.r = d.g = d.b = s;
            }
        }
        break;

    case 3:
		return;

    case 4:
		// Strip alpha
        {            
            ImageBuffer::Ref old = m_buffer;
            Color4unorm8* src = pixel4();
            resize(width(), height(), 3);
            for (int i = width() * height() - 1; i >= 0; --i) {
                const Color4unorm8   s = src[i];
                Color3unorm8&        d = pixel3()[i]; 
                d.r = s.r;
                d.g = s.g;
                d.b = s.b;
            }
        }
        break;

    default:
        alwaysAssertM(false, "Bad number of channels in input image");
    }
}


void GImage::R8G8B8_to_Y8U8V8(int width, int height, const unorm8* _in, unorm8* _out) {
    const Color3unorm8* in = reinterpret_cast<const Color3unorm8*>(_in);
    Color3unorm8* out = reinterpret_cast<Color3unorm8*>(_out);

    Color3unorm8 p;
    for (int i = width * height - 1; i >= 0; --i) {
        const Color3 src(*in);
        p.r = unorm8(src.r *  0.229f + src.g *  0.587f + src.b *  0.114f);
        p.g = unorm8(src.r * -0.147f + src.g * -0.289f + src.b *  0.436f);
        p.b = unorm8(src.r *  0.615f + src.g * -0.515f + src.b * -0.100f);
        *out = p;
        ++in;
        ++out;
    }
}


void GImage::Y8U8V8_to_R8G8B8(int width, int height, const unorm8* _in, unorm8* _out) {
    const Color3unorm8* in = reinterpret_cast<const Color3unorm8*>(_in);
    Color3unorm8* out = reinterpret_cast<Color3unorm8*>(_out);

    Color3unorm8 p;
    for (int i = width * height - 1; i >= 0; --i) {
        const Color3 src(*in);
        p.r = unorm8(src.r *  1.0753f + (src.b - 0.5f) * 1.2256f);
        p.g = unorm8(src.r *  1.0753f + (src.g - 0.5f) * -0.3946f + (src.b - 0.5f) * -0.4947f);
        p.b = unorm8(src.r *  1.0753f + (src.g - 0.5f) *  2.0320f + (src.b - 0.5f) *  0.0853f);
        *out = p;
        ++in;
        ++out;
    }
}


void GImage::makeCheckerboard(GImage& im, int checkerSize, const Color4unorm8& A, const Color4unorm8& B) {
    for (int y = 0; y < im.height(); ++y) {
        for (int x = 0; x < im.width(); ++x) {
            bool checker = isOdd((x / checkerSize) + (y / checkerSize));
            const Color4unorm8& color = checker ? A : B;
            for (int c = 0; c < im.channels(); ++c) {
                uint8* v = im.byte() + (x + y * im.width()) * im.channels() + c;
                *v = color[c];
            }
        }
    }
}

}

