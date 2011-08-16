/**
  \file Image3unorm8.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-01-31
  \edited  2011-06-27
*/

#include "G3D/Image1unorm8.h"
#include "G3D/Image3unorm8.h"
#include "G3D/Image3.h"
#include "G3D/Image4.h"
#include "G3D/GImage.h"
#include "G3D/Color1.h"
#include "G3D/Color1unorm8.h"
#include "G3D/Color4.h"
#include "G3D/Color4unorm8.h"
#include "G3D/ImageFormat.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

void Image3unorm8::speedSerialize(class BinaryOutput& b) const {
    b.writeInt32(w);
    b.writeInt32(h);
    _wrapMode.serialize(b);
    b.writeInt32(ImageFormat::CODE_RGB8);
    
    // Write the data
    GImage temp(GImage::SHARE_DATA, (uint8*)data.getCArray(), w, h, format());
    temp.encode(GImage::PNG, b);
}

    
Image3unorm8::Ref Image3unorm8::speedCreate(class BinaryInput& b) {
    const int w = b.readInt32();
    const int h = b.readInt32();
    WrapMode wrap;
    wrap.deserialize(b);

    ImageFormat::Code fmt = ImageFormat::Code(b.readInt32());

    alwaysAssertM(fmt == ImageFormat::CODE_RGB8, 
                  G3D::format("Cannot SpeedCreate an Image3unorm8 from %s",
                         ImageFormat::fromCode(fmt)->name().c_str()));

    Ref im = createEmpty(w, h, wrap);

    // Read the data (note that this code would not work for R8() data due to a bug in the PNG loader as of July 4, 2011, but it will work for 
    // RGB8 data)
    GImage temp(GImage::SHARE_DATA, (uint8*)im->data.getCArray(), im->w, im->h, im->format());
    temp.decode(b, GImage::PNG);
    alwaysAssertM(im->data.getCArray() == temp.pixel3(), "GImage::decode failed to use the memory provided");

    return im;
}

    

Image3unorm8::Ref Image3unorm8::fromImage1unorm8(const ReferenceCountedPointer<class Image1unorm8>& im) {
    return fromArray(im->getCArray(), im->width(), im->height(), im->wrapMode());
}


Image3unorm8::Image3unorm8(int w, int h, WrapMode wrap) : Map2D<Color3unorm8, Color3>(w, h, wrap) {
    setAll(Color3unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero()));
}


Image3unorm8::Ref Image3unorm8::fromGImage(const GImage& im, WrapMode wrap) {
    switch (im.channels()) {
    case 1:
        return fromArray(im.pixel1(), im.width(), im.height(), wrap);

    case 3:
        return fromArray(im.pixel3(), im.width(), im.height(), wrap);

    case 4:
        return fromArray(im.pixel4(), im.width(), im.height(), wrap);

    default:
        debugAssertM(false, "Input GImage must have 1, 3, or 4 channels.");
        return NULL;
    }
}


Image3unorm8::Ref Image3unorm8::fromImage3(const ReferenceCountedPointer<Image3>& im) {
    return fromArray(im->getCArray(), im->width(), im->height(), static_cast<WrapMode>(im->wrapMode()));
}


Image3unorm8::Ref Image3unorm8::fromImage4(const ReferenceCountedPointer<Image4>& im) {
    return fromArray(im->getCArray(), im->width(), im->height(), static_cast<WrapMode>(im->wrapMode()));
}



Image3unorm8::Ref Image3unorm8::createEmpty(int width, int height, WrapMode wrap) {
    return new Type(width, height, wrap);
}


Image3unorm8::Ref Image3unorm8::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap);
}


Image3unorm8::Ref Image3unorm8::fromFile(const std::string& filename, WrapMode wrap, GImage::Format fmt) {
    Ref out = createEmpty(wrap);
    out->load(filename, fmt);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color3unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color1* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color1unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color3* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color4unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image3unorm8::Ref Image3unorm8::fromArray(const class Color4* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


void Image3unorm8::load(const std::string& filename, GImage::Format fmt) {
    copyGImage(GImage(filename, fmt));
    setChanged(true);
}


void Image3unorm8::copyGImage(const GImage& im) {
    switch (im.channels()) {
    case 1:
        copyArray(im.pixel1(), im.width(), im.height());
        break;

    case 3:
        copyArray(im.pixel3(), im.width(), im.height());
        break;

    case 4:
        copyArray(im.pixel4(), im.width(), im.height());
        break;
    } 
}


void Image3unorm8::copyArray(const Color1unorm8* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color3unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = src[i].value;
    }
}

void Image3unorm8::copyArray(const Color1* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color3unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = Color1unorm8(src[i]).value;
    }
}


void Image3unorm8::copyArray(const Color3unorm8* ptr, int w, int h) {
    resize(w, h);
    System::memcpy(getCArray(), ptr, w * h * 3);
}


void Image3unorm8::copyArray(const Color3* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color3unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color3unorm8(src[i]);
    }
}


void Image3unorm8::copyArray(const Color4unorm8* ptr, int w, int h) {
    resize(w, h);
    
    // Copy 3/4 bytes
    GImage::RGBAtoRGB((const unorm8*)ptr, (unorm8*)getCArray(), w * h);
}


void Image3unorm8::copyArray(const Color4* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color3unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color3unorm8(src[i].rgb());
    }
}


/** Saves in any of the formats supported by G3D::GImage. */
void Image3unorm8::save(const std::string& filename, GImage::Format fmt) {
    GImage im(width(), height(), 3);
    System::memcpy(im.byte(), getCArray(), width() * height() * 3);
    im.save(filename, fmt);
}


ReferenceCountedPointer<class Image1unorm8> Image3unorm8::getChannel(int c) const {
    debugAssert(c >= 0 && c <= 2);

    Image1unorm8Ref dst = Image1unorm8::createEmpty(width(), height(), wrapMode());
    const Color3unorm8* srcArray = getCArray();
    Color1unorm8* dstArray = dst->getCArray();

    const int N = width() * height();
    for (int i = 0; i < N; ++i) {
        dstArray[i] = Color1unorm8(srcArray[i][c]);
    }

    return dst;
}


const ImageFormat* Image3unorm8::format() const {
    return ImageFormat::RGB8();
}

} // G3D