//
// Created by ZhangHao on 2022/12/1.
//

#include "SmoothKernels.h"

using namespace SmoothKernels;

vec2 poly6(vec2 &delta_r, float h)
{
    return vec2(0, 0);
}

vec2 d_poly6(vec2 &delta_r, float h)
{
    return vec2(0, 0);
}

vec2 dd_poly6(vec2 &delta_r, float h)
{
    return vec2(0, 0);
}

static SmoothKernel s_poly6(poly6, d_poly6, dd_poly6);
SmoothKernel &SmoothKernels::Poly6() {
    return s_poly6;
}

SmoothKernel &SmoothKernels::DebrunSpiky() {
    return s_poly6;
}

