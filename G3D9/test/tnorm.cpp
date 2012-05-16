#include "G3D/unorm8.h"
#include "G3D/unorm16.h"
#include "G3D/snorm8.h"
#include "G3D/snorm16.h"

using namespace G3D;

float maxRoundOffError(float x) {
    // Next representable floating point number
    uint32 y = *(uint32*)(&x);
    ++y;

    float z = *(float*)(&y);

    // Previous representable number
    y -= 2;
    float w = *(float*)(&y);

    return max(z-x, x-w);
}


float getULP() {

    int N = 10000;
    float maxULP = 0.0f;
    // Make a lot of floating point numbers between 0 and 1
    for (int i = 0; i < N; ++i) {
        const float f1 = (i / float(N - 1) + 0.5f) * 2.0f;
        float ulp = maxRoundOffError(f1);
        if (ulp > maxULP) {
            maxULP = ulp;
        }
    }

    return maxULP;
}


void testunorm8() {
    // This isn't REALLY one ULP; it is a magic epsilon value.  But where we use it, we should have
    // used 1 ULP, and this is conservatively larger for -1 <= x <= 1
    const float ULP = getULP();

    printf("unorm8 ");

    int N = 10000;
    // Make a lot of floating point numbers between 0 and 1
    for (int i = 0; i < N; ++i) {
        const float f1 = i / float(N - 1);

        unorm8 x(f1);

        const float f2 = x;
        const float error = abs(f1 - f2);

        alwaysAssertM(error <= 0.5 / 255.0 + ULP, format("error = %f", error));
    }

    // Ensure that 1.0 and 0.0 are exact
    alwaysAssertM(float(unorm8(1.0f)) == 1.0f, "1.0f was not exactly representable");
    alwaysAssertM(float(unorm8(0.0f)) == 0.0f, "0.0f was not exactly representable");

    // TODO: verify that every number is closer than the adjacent representations

    printf("passed\n");
}


void testunorm16() {
    printf("unorm16 ");
    // This isn't REALLY one ULP; it is a magic epsilon value.  But where we use it, we should have
    // used 1 ULP, and this is conservatively larger for -1 <= x <= 1
    const float ULP = getULP();

    const int N = 100000;
    // Make a lot of floating point numbers between 0 and 1
    for (int i = 0; i < N; ++i) {
        const float f1 = i / float(N - 1);

        unorm16 x(f1);

        const float f2 = x;
        const float error = abs(f1 - f2);
		 
        alwaysAssertM((error - 0.5 / 65535.0) <= ULP, format("error = %f", error));
    }

    // Ensure that 1.0 and 0.0 are exact
    alwaysAssertM(float(unorm16(1.0f)) == 1.0f, "1.0f was not exactly representable");
    alwaysAssertM(float(unorm16(0.0f)) == 0.0f, "0.0f was not exactly representable");

    printf("passed\n");
}


void testsnorm8() {
    float ULP = getULP();
    //0.0001f;

    printf("snorm8 ");

    int N = 10000;
    // Make a lot of floating point numbers between -1 and 1
    for (int i = 0; i < N; ++i) {
        const float f1 = (i / float(N - 1) - 0.5f) * 2.0f;

        const snorm8 x(f1);

        const float f2 = x;
        const float error = abs(f1 - f2);
		 
        alwaysAssertM( error <= 0.5 / 127.0 + ULP, format("error = %f  %f - > %f", error, f1, f2));
    }

    // Ensure that 1.0 and 0.0 are exact
    alwaysAssertM(float(snorm8(1.0f)) == 1.0f, "1.0f was not exactly representable");
    alwaysAssertM(float(snorm8(0.0f)) == 0.0f, "0.0f was not exactly representable");

    printf("passed\n");
}


void testsnorm16() {
    printf("snorm16 ");
    // This isn't REALLY one ULP; it is a magic epsilon value.  But where we use it, we should have
    // used 1 ULP, and this is conservatively larger for -1 <= x <= 1
    float ULP = getULP();

    int N = 10000;
    // Make a lot of floating point numbers between 0 and 1
    for (int i = 0; i < N; ++i) {
        const float f1 = i / float(N);

        snorm16 x(f1);

        const float f2 = x;
        const float error = abs(f1 - f2);
		 
        alwaysAssertM((error - 0.5 / 32767.0) <= ULP, format("error = %f", error));
    }

    // Ensure that 1.0 and 0.0 are exact
    alwaysAssertM(float(snorm16(1.0f)) == 1.0f, "1.0f was not exactly representable");
    alwaysAssertM(float(snorm16(0.0f)) == 0.0f, "0.0f was not exactly representable");

    printf("passed\n");
}
