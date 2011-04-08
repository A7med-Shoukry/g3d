#include "G3D/G3D.h"
using namespace G3D;

void testfilter() {
    printf("G3D::gaussian1D  ");
    Array<float> coeff;

    gaussian1D(coeff, 5, 0.5f);
    debugAssert(fuzzyEq(coeff[0], 0.00026386508f));
    debugAssert(fuzzyEq(coeff[1], 0.10645077f));
    debugAssert(fuzzyEq(coeff[2], 0.78657067f));
    debugAssert(fuzzyEq(coeff[3], 0.10645077f));
    debugAssert(fuzzyEq(coeff[4], 0.00026386508f));

    printf("passed\n");
}
