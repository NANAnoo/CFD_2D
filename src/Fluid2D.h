#ifndef FLUID_2D_H
#define FLUID_2D_H

#include "GLRenderable.h"
#include "Vec3.h"
#include <type_traits>

class Fluid2D : public GLRenderableI
{
public:
    struct Vec2
    {
    public:
        float x;
        float y;
        Vec2():x(0), y(0){}
        Vec2(float a, float b):x(a), y(b){}
    };

    enum KernalForm
    {
        ORIGIN = 0,
        DIFF = 1,
        LAPLACE = 2
    };

    typedef Vec2 (*kernel_function)(Vec2 &delta_r, float h);

    struct SmoothKernal
    {
    private:
        // W(r, h)
        kernel_function func;
        // diff W(r, h)
        kernel_function d_func;
        // diff diff W(r, h)
        kernel_function dd_func;
    public:
        template <KernalForm form>
        Vec2 eval(Vec2 &delta_r, float h) {
            if (form == ORIGIN) {
                return func(delta_r, h);
            } else if (form == DIFF) {
                return d_func(delta_r, h);
            } else if (form == LAPLACE) {
                return dd_func(delta_r, h);
            }
        }
        template <KernalForm form>
        void set(kernel_function f) {
            if (form == ORIGIN) {
                func = f;
            } else if (form == DIFF) {
                d_func = f;
            } else if (form == LAPLACE) {
                dd_func = f;
            }
        }
    };
    
    
    struct Fluid2DParameters
    {
        // boundary;
        float top;
        float buttom;
        float left;
        float right;
        // grid_size, N * N grid;
        unsigned int grid_size;

        // particle infos
        unsigned int particle_count;
        float particle_mass;

        // fluid properties
        // initial density
        float rho_0;
        // pressure constant
        float K;
        // viscosity coefficent, disabled when V == 0
        float V;
        // surface tension, disabled when σ == 0
        float sigma;
        
        // callbacks
        // initial 
        void (*init_positions)(std::vector<Vec2> &positions);
        // smooth kernals
        SmoothKernal pressure_kernal;
        SmoothKernal viscosity_kernal;
        SmoothKernal surface_tension_kernal;

        Fluid2DParameters()
        {
            // default values;
        }
    };
    
    Fluid2D(Fluid2DParameters &params);
    ~Fluid2D();

    // update
    void update();

private:
    Fluid2DParameters params;
    // initialize
    void init();
    // render fluid
    void render();
};

#endif FLUID_2D_H