//
// Created by ZhangHao on 2022/12/1.
//

#ifndef CFD_2D_SMOOTH_KERNELS_H
#define CFD_2D_SMOOTH_KERNELS_H

#include "Vec.h"

#define diff eval<SmoothKernels::DIFF>
#define laplace eval<SmoothKernels::LAPLACE>

namespace SmoothKernels {
    enum KernelForm {
        ORIGIN = 0,
        DIFF = 1,
        LAPLACE = 2
    };
    // kernel return a vector
    template<VectorSize size>
    using v_kernel_function = Vec<size>(Vec<size> &r);
    // a kernel return a float value
    template<VectorSize size>
    using kernel_function = float(Vec<size> &r);

    template<VectorSize size>
    struct SmoothKernel {
    private:
        // W(r, h), return a float
        kernel_function<size> *func;
        // diff W(r, h), return a vector
        v_kernel_function<size> *d_func;
        // diff dot diff W(r, h), return a float
        kernel_function<size> *dd_func;
    public:
        auto operator()(Vec<size> &delta_r) {
            return (*func)(delta_r);
        }

        template<KernelForm form>
        auto eval(Vec<size> &delta_r) {
            if constexpr (form == ORIGIN) {
                return (*func)(delta_r);
            } else if constexpr (form == DIFF) {
                return (*d_func)(delta_r);
            } else if constexpr (form == LAPLACE) {
                return (*dd_func)(delta_r);
            }
        }

        SmoothKernel(kernel_function<size> f, v_kernel_function<size> df, kernel_function<size> lf) {
            this->func = f;
            this->d_func = df;
            this->dd_func = lf;
        }
    };
}
#endif //CFD_2D_SMOOTH_KERNELS_H

// default implementation
#ifdef KERNEL_WITH_H
#include "SmoothKernelIMPL.h"
// extern poly6, spiky, viscosity
KERNEL_EXTERN(poly6)
KERNEL_EXTERN(spiky)
KERNEL_EXTERN(viscosity)
// default kernels
template<VectorSize size> SmoothKernels::SmoothKernel<size> &Poly6()
{
    static SmoothKernels::SmoothKernel<size> s_poly6(poly6<size>, d_poly6<size>, dd_poly6<size>);
    return s_poly6;
}

template<VectorSize size> SmoothKernels::SmoothKernel<size> &DebrunSpiky()
{
    static SmoothKernels::SmoothKernel<size> s_spiky(spiky<size>, d_spiky<size>, dd_spiky<size>);
    return s_spiky;
}

template<VectorSize size> SmoothKernels::SmoothKernel<size> &Viscosity()
{
    static SmoothKernels::SmoothKernel<size> s_viscosity(viscosity<size>, d_viscosity<size>, dd_viscosity<size>);
    return s_viscosity;
}
#endif // KERNEL_WITH_H