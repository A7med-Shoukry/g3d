/**
  \file G3D/ImageConvert.cpp

  \created 2012-05-24
  \edited  2012-05-24
*/

#include "G3D/ImageConvert.h"


namespace G3D {


ImageBuffer::Ref ImageConvert::convertBuffer(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    // Early return for no conversion
    if (src->format() == dstFormat)
    {
        return src;
    }

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
    // Only handle inter-RGB color space conversions for now
    if (src->format()->colorSpace != ImageFormat::COLOR_SPACE_RGB)
    {
        return NULL;
    }

    if (dstFormat->colorSpace != ImageFormat::COLOR_SPACE_RGB)
    {
        return NULL;
    }

    // Check for color order reversal
    if (src->format()->code == ImageFormat::CODE_RGBA8 && dstFormat->code == ImageFormat::CODE_BGRA8)
    {
        return convertRGBA8toBGRA8;
    }

    // Check for conversion that only adds alpha channel
    if (ImageFormat::getFormatWithAlpha(src->format()) == dstFormat)
    {
        return &ImageConvert::convertRGBAddAlpha;
    }

    return NULL;
}

ImageBuffer::Ref ImageConvert::convertRGBAddAlpha(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    debugAssert(src->rowAlignment() == 1);

    ImageBuffer::Ref dstImage = ImageBuffer::create(src->width(), src->height(), dstFormat);
    for (int pixelIndex = 0; pixelIndex = src->width() * src->height(); ++pixelIndex)
    {
        switch (dstFormat->code)
        {
            case ImageFormat::CODE_RGBA8:
            {
                uint8* oldPixels = static_cast<uint8*>(src->buffer());
                uint8* newPixels = static_cast<uint8*>(dstImage->buffer());

                newPixels[pixelIndex * 4] = oldPixels[pixelIndex * 3];
                newPixels[pixelIndex * 4 + 1] = oldPixels[pixelIndex * 3 + 1];
                newPixels[pixelIndex * 4 + 2] = oldPixels[pixelIndex * 3 + 2];
                newPixels[pixelIndex * 4 + 3] = 0;
                break;
            }
            default:
                debugAssertM(false, "Unsupported destination image format");
                break;
        }
    }

    return dstImage;
}

ImageBuffer::Ref ImageConvert::convertRGBA8toBGRA8(const ImageBuffer::Ref& src, const ImageFormat* dstFormat) {
    int width = src->width();
    int height = src->height();

    ImageBuffer::Ref dstBuffer = ImageBuffer::create(width, height, dstFormat, AlignedMemoryManager::create());

    unorm8* srcData = static_cast<unorm8*>(src->buffer());
    unorm8* dstData = static_cast<unorm8*>(dstBuffer->buffer());

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

} // namespace G3D
