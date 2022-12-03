//
// Created by ZhangHao on 2022/12/1.
//
#ifndef SMOOTH_KERNEL_IMPL
#define SMOOTH_KERNEL_IMPL

#include "Vec.h"
#include <unordered_map>
#include <iostream>

using namespace std;

/*--------------------  COMPILE TIME VARIABLES -------------------*/

// optimize everything during compiling

template<int order>
constexpr float c_pow(float x) noexcept {
    return c_pow<(order + 1) / 2>(x) * c_pow<order / 2>(x);
}

template<>
constexpr float c_pow<1>(float x) noexcept {
    return x;
}

template<>
constexpr float c_pow<0>(float x) noexcept {
    return 1.f;
}

#define PI 3.1415926535f

/*---------------- KERNEL IMPLEMENTATION MACROS -----------------*/
/* usage:   */
/* KERNEL_IMPL(name, $(scale_expr, function_expr)) */
/* KERNEL_EXTERN(name) in *.cpp                    */
#define KERNEL_IMPL(name, scale_, statements, scale_d, d_statements, scale_dd, dd_statements) \
static unordered_map<float, float> name##_scales, name##_scales_d, name##_scales_dd;          \
constexpr float name##_SCALE(float h) { return (scale_);}                                     \
constexpr float name##_SCALE_D(float h) { return (scale_d);}                                  \
constexpr float name##_SCALE_DD(float h) { return (scale_dd);}                                \
    template <VectorSize size>\
    float name(Vec<size> &r, const float h) {                      \
        float length = r.length();                                 \
        if (length > h) return 0;                                  \
        if (name##_scales.find(h) == name##_scales.end()) {        \
            name##_scales[h] = name##_SCALE(h);                    \
        }                                                          \
        const float scale = name##_scales[h];                      \
        statements                                                 \
    }                                                              \
    template <VectorSize size>\
    Vec<size> d_##name(Vec<size> &r, const float h) {              \
        float length = r.length();                                 \
        if (length > h) return Vec<size>();                        \
        if (name##_scales_d.find(h) == name##_scales_d.end()) {    \
            name##_scales_d[h] = name##_SCALE_D(h);                \
        }                                                          \
        const float scale = name##_scales_d[h];                    \
        d_statements                                               \
    }                                                              \
    template <VectorSize size>\
    float dd_##name(Vec<size> &r, const float h) {                 \
        float length = r.length();                                 \
        if (length > h) return 0;                                  \
        if (name##_scales_dd.find(h) == name##_scales_dd.end()) {  \
            name##_scales_dd[h] = name##_SCALE_DD(h);              \
        }                                                          \
        const float scale = name##_scales_dd[h];                   \
        dd_statements                                              \
    }

#define KERNEL_EXTERN(name) \
    template <VectorSize size> extern float name(Vec<size> &r, const float h); \
    template <VectorSize size> extern Vec<size> d_##name(Vec<size> &r, const float h); \
    template <VectorSize size> extern float dd_##name(Vec<size> &r, const float h);

/*--------------------  POLY6 IMPLEMENTATION ---------------------*/
// return 0 if |r| < h
// else return 315 / ( 64 * PI * h^9 ) * ( h^2 - r^2 ) ^ 3
// implementation of poly6
KERNEL_IMPL
(poly6,
// 315 / ( 64 * PI * h^9 ) * (h^2 - |r|^2)^3
 315.f / (64.f * PI * c_pow<9>(h)),
 {
     float sub = h * h - length * length;
     return scale * sub * sub * sub;
 },
// r * (-945/ (32 * pi * h^9)) * (h^2 - |r|^2)^2
 -945.f / (32.f * PI * c_pow<9>(h)),
 {
     float sub = h * h - length * length;
     return r * (scale * sub * sub);
 },
// (945/ (8 * pi * h^9)) * (h^2 - |r|^2) * ( |r|^2 - 3/4 * (h^2 - |r|^2))
 945.f / (8.f * PI * c_pow<9>(h)),
 {
     float length_2 = length * length;
     float sub = h * h - length_2;
     return scale * sub * (length_2 - 0.75 * sub);
 })

/*--------------------  spiky IMPLEMENTATION ---------------------*/
// return 0 if |r| < h
// else return 15 / pi * h^6 * (h - r) ^ 3
// implementation of Debruns' Spiky
KERNEL_IMPL
(spiky,
// 15 / (pi * h^6) * (h - |r|) ^ 3
 15 / (PI * c_pow<6>(h)),
 {
     float sub = h - length;
     return scale * sub * sub * sub;
 },
// -r * (45 / pi * h^6 * |r|) * (h - |r|)^2
 -45 / (PI * c_pow<6>(h)),
 {
     if (length < std::numeric_limits<float>::epsilon()) {
         return Vec<size> ();
     }
     float sub = h - length;
     return r * (scale / length * sub * sub);
 },
// - 90 / (pi * h ^ 6 * |r|) * (h - |r|)* (h - 2 * |r|)
 -90 / (PI * c_pow<6>(h)),
 {
     return scale / length * (h - length) * (h - 2 * length);
 })

/*--------------------  spiky IMPLEMENTATION ---------------------*/
KERNEL_IMPL
(viscosity,
 // 15 / (2 * pi * h ^ 3) * ( - |r|^3 / h^3 + |r| ^ 2 / h ^ 2 + h / (2 * |r|) - 1)
 15 / (2 * PI * c_pow<3>(h)),
 {
     float r_d_h = length / h;
     return scale * (-0.5 * r_d_h * r_d_h * r_d_h + r_d_h * r_d_h + 0.5 / r_d_h - 1);
 },
// r * 15 / (2 * pi * h ^ 3) * ( - 3 * |r|^2 / (2 * h^3) + 2 / h ^ 2 - h / (2 * |r|^3))
 15 / (2 * PI * c_pow<3>(h)),
 {
    if (length < std::numeric_limits<float>::epsilon()) {
        return Vec<size> ();
    }
    float const_value = scale * ( - 3 * length / (2 * h * h * h)
                                + 2 / (h * h)
                                - h / (2 * length * length * length));
    return r * const_value;
 },
 // 45 / (pi * h^6) * (h - |r|)
 45 / (PI * c_pow<6>(h)),
 {
    return scale * (h - length);
 })

#endif // SMOOTH_KERNEL_IMPL