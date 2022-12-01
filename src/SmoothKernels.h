//
// Created by ZhangHao on 2022/12/1.
//

#ifndef CFD_2D_SMOOTH_KERNELS_H
#define CFD_2D_SMOOTH_KERNELS_H

#include "Vec.h"

namespace SmoothKernels {
    enum KernelForm
    {
        ORIGIN = 0,
        DIFF = 1,
        LAPLACE = 2
    };
    typedef vec2 (*kernel_function)(vec2 &delta_r, float h);
    struct SmoothKernel
    {
    private:
        // W(r, h)
        kernel_function func;
        // diff W(r, h)
        kernel_function d_func;
        // diff diff W(r, h)
        kernel_function dd_func;
    public:
        template <KernelForm form>
        vec2 eval(vec2 &delta_r, float h) {
            if (form == ORIGIN) {
                return func(delta_r, h);
            } else if (form == DIFF) {
                return d_func(delta_r, h);
            } else if (form == LAPLACE) {
                return dd_func(delta_r, h);
            }
        }
        template <KernelForm form>
        void set(kernel_function f) {
            if (form == ORIGIN) {
                func = f;
            } else if (form == DIFF) {
                d_func = f;
            } else if (form == LAPLACE) {
                dd_func = f;
            }
        }
        SmoothKernel(kernel_function f, kernel_function df, kernel_function lf):
            func(f), d_func(df), dd_func(lf)
        {}
    };
    SmoothKernel &Poly6();
    SmoothKernel &DebrunSpiky();
};


#endif //CFD_2D_SMOOTH_KERNELS_H
