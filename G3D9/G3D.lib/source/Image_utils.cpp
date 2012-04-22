/**
  \file Image_utils.cpp
  \author Corey Taylor
  Copyright 2002-2012, Morgan McGuire

  \created 2002-05-27
  \edited  2012-04-21
 */
#include "G3D/platform.h"
#include "G3D/Image.h"

namespace G3D {

Image::Ref Image::computeNormalMap
(int                 width,
 int                 height,
 int                 channels,
 const unorm8*       src,
 const BumpMapPreprocess& preprocess) {

#if 0
    Ref normal = ImageBuffer::create(width, height, ImageFormat::RGBA8());
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
#endif
    return NULL;
}


} // namespace G3D

