//
// Created by ZhangHao on 2022/12/1.
//

#ifdef KERNEL_WITH_H
const float H = KERNEL_WITH_H;
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
constexpr float name##_SCALE(float h) { return (scale_);}                                     \
constexpr float name##_SCALE_D(float h) { return (scale_d);}                                  \
constexpr float name##_SCALE_DD(float h) { return (scale_dd);}                                \
    template <VectorSize size>\
    float name(Vec<size> &r) {                      \
        float length = r.length();                                 \
        if (length > H) return 0;                                  \
        const float scale = name##_SCALE(H);                      \
        statements                                                 \
    }                                                              \
    template <VectorSize size>\
    Vec<size> d_##name(Vec<size> &r) {              \
        float length = r.length();                                 \
        if (length > H) return Vec<size>();                        \
        const float scale = name##_SCALE_D(H);                    \
        d_statements                                               \
    }                                                              \
    template <VectorSize size>\
    float dd_##name(Vec<size> &r) {                 \
        float length = r.length();                                 \
        if (length > H) return 0;                                  \
        const float scale = name##_SCALE_DD(H);                   \
        dd_statements                                              \
    }

#define KERNEL_EXTERN(name) \
    template <VectorSize size> extern float name(Vec<size> &r); \
    template <VectorSize size> extern Vec<size> d_##name(Vec<size> &r); \
    template <VectorSize size> extern float dd_##name(Vec<size> &r);

/*--------------------  POLY6 IMPLEMENTATION ---------------------*/
// return 0 if |r| < h
// else return 315 / ( 64 * PI * h^9 ) * ( h^2 - r^2 ) ^ 3
// implementation of poly6
KERNEL_IMPL
(poly6,
// 315 / ( 64 * PI * h^9 ) * (h^2 - |r|^2)^3
 315.f / (64.f * PI * c_pow<9>(h)),
 {
     float sub = c_pow<2>(H) - length * length;
     return scale * sub * sub * sub;
 },
// r * (-945/ (32 * pi * h^9)) * (h^2 - |r|^2)^2
 -945.f / (32.f * PI * c_pow<9>(h)),
 {
     float sub = c_pow<2>(H) - length * length;
     return r * (scale * sub * sub);
 },
// (945/ (8 * pi * h^9)) * (h^2 - |r|^2) * ( |r|^2 - 3/4 * (h^2 - |r|^2))
 945.f / (8.f * PI * c_pow<9>(h)),
 {
     float length_2 = length * length;
     float sub = c_pow<2>(H) - length_2;
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
     float sub = H - length;
     return scale * sub * sub * sub;
 },
// -r * (45 / pi * h^6 * |r|) * (h - |r|)^2
 -45 / (PI * c_pow<6>(h)),
 {
     if (length < std::numeric_limits<float>::epsilon()) {
         return Vec<size> ();
     }
     float sub = H - length;
     return r * (scale / length * sub * sub);
 },
// - 90 / (pi * h ^ 6 * |r|) * (h - |r|)* (h - 2 * |r|)
 -90 / (PI * c_pow<6>(h)),
 {
     return scale / length * (H - length) * (H - 2 * length);
 })

/*--------------------  spiky IMPLEMENTATION ---------------------*/
KERNEL_IMPL
(viscosity,
 // 15 / (2 * pi * h ^ 3) * ( - |r|^3 / h^3 + |r| ^ 2 / h ^ 2 + h / (2 * |r|) - 1)
 15 / (2 * PI * c_pow<3>(h)),
 {
     float r_d_h = length / H;
     return scale * (-0.5 * r_d_h * r_d_h * r_d_h + r_d_h * r_d_h + 0.5 / r_d_h - 1);
 },
// r * 15 / (2 * pi * h ^ 3) * ( - 3 * |r|^2 / (2 * h^3) + 2 / h ^ 2 - h / (2 * |r|^3))
 15 / (2 * PI * c_pow<3>(h)),
 {
    if (length < std::numeric_limits<float>::epsilon()) {
        return Vec<size> ();
    }
    float const_value = scale * ( - 3 * length / (2 * c_pow<3>(H))
                                + 2 / c_pow<2>(H)
                                - H / (2 * length * length * length));
    return r * const_value;
 },
 // 45 / (pi * h^6) * (h - |r|)
 45 / (PI * c_pow<6>(h)),
 {
    return scale * (H - length);
 })

#endif // H