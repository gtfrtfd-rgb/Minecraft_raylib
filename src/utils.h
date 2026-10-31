#pragma once

#include <cmath>

namespace Utils {
    float Noise2D(float x, float y);
    float Fract(float v);
    float Lerp(float a, float b, float t);
    unsigned int TreeHash(int x, int z); 
}