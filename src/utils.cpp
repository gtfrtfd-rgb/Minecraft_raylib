#include "utils.h"
#include <cstdlib>

namespace Utils {

static unsigned int Hash(int x, int y) {
    unsigned int n = (unsigned int)x + (unsigned int)y * 57U;
    n = (n << 13U) ^ n;
    return (n * (n * n * 15731U + 789221U) + 1376312589U);
}

float Fract(float v) { return v - floorf(v); }
float Lerp(float a, float b, float t) { return a + t * (b - a); }

float Noise2D(float x, float y) {
    int xi = static_cast<int>(floorf(x)) & 255;
    int yi = static_cast<int>(floorf(y)) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);

    auto Fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    auto Grad = [](unsigned int hash, float x, float y) {
        unsigned int h = hash & 7U;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1U) ? -u : u) + ((h & 2U) ? -2.0f * v : 2.0f * v);
    };

    float u = Fade(xf);
    float v = Fade(yf);

    unsigned int aa = Hash(xi, yi);
    unsigned int ab = Hash(xi, yi + 1);
    unsigned int ba = Hash(xi + 1, yi);
    unsigned int bb = Hash(xi + 1, yi + 1);

    float x1 = Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u);
    float x2 = Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u);

    return Lerp(x1, x2, v);
}

unsigned int TreeHash(int x, int z) {
    unsigned int h = (unsigned int)x * 374761393U + (unsigned int)z * 668265263U;
    h = (h ^ (h >> 13U)) * 1274126177U;
    return h ^ (h >> 16U);
}

} // namespace Utils