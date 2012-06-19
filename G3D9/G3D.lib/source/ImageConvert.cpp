/**
  \file G3D/ImageConvert.cpp

  \created 2012-05-24
  \edited  2012-05-24
*/

#include "G3D/ImageConvert.h"


namespace G3D {



static ImageBuffer::Ref convertRGBA8toBGRA8(const ImageBuffer::Ref& src, const ImageFormat* dstFormat){
    alwaysAssertM(src->format() == ImageFormat::RGBA8() && dstFormat == ImageFormat::BGRA8(), 
        format("This conversion only works from RGBA8 to BGRA8, not %s to %s", src->format()->name().c_str(), dstFormat->name().c_str()));
    int width = src->width();
    int height = src->height();

    ImageBuffer::Ref dstBuffer = ImageBuffer::create(width, height, dstFormat, AlignedMemoryManager::create(), 1, 1);


    unorm8* srcData = (unorm8*)src->buffer();
    unorm8* dstData = (unorm8*)dstBuffer->buffer();

    const int bytesPerPixel = 4;

    // From RGBA to BGRA, for every 4 bytes, first and third swapped, others remain in place
    for(int i = 0; i < (bytesPerPixel * width * height); i += 4) {
        dstData[i+0] = srcData[i+2];
        dstData[i+1] = srcData[i+1];
        dstData[i+2] = srcData[i+0];
        dstData[i+3] = srcData[i+3];
    }

    return dstBuffer;



}

static ImageBuffer::Ref convertRGBAddAlpha(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    return NULL;
}

ImageBuffer::Ref ImageConvert::convertBuffer(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    ConvertFunc converter = findConverter(src, dstFormat);
    if (converter)
    {
        return (*converter)(src, dstFormat);
    }
    else
    {
        return NULL;
    }
}

ImageConvert::ConvertFunc ImageConvert::findConverter(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    if(src->format() == ImageFormat::RGBA8() && dstFormat == ImageFormat::BGRA8()){ //TODO: Allow the reverse transformation (which is the same)
        return convertRGBA8toBGRA8;
    } else {
        return NULL;
    }
}



} // namespace G3D
