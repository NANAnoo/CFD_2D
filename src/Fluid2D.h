#ifndef FLUID_2D_H
#define FLUID_2D_H

#include "GLRenderable.h"
#include "Vec.h"
#include <vector>
#include <type_traits>
#include "SmoothKernels.h"

class Fluid2D : public GLRenderableI
{
    struct Fluid2DParameters
    {
        // boundary;
        float top;
        float bottom;
        float left;
        float right;
        // grid_size, N * N grid;
        unsigned int grid_size;
        // gravity
        vec2 gravity;

        // particle infos
        unsigned int particle_count;
        float particle_mass;

        // fluid properties
        // initial density
        float rho_0;
        // pressure constant
        float K;
        // viscosity coefficient, disabled when V == 0
        float V;
        // surface tension, disabled when σ == 0
        float sigma;
        
        // callbacks
        // initial 
        void (*init_positions)(std::vector<vec2> &positions);
        // smooth kernels
        SmoothKernels::SmoothKernel<D2> *pressure_kernel;
        SmoothKernels::SmoothKernel<D2> *viscosity_kernel;
        SmoothKernels::SmoothKernel<D2> *surface_tension_kernel;

        Fluid2DParameters() :
                top(1), bottom(-1), left(-1), right(1), grid_size(20),
                particle_count(1000), particle_mass(1), gravity(vec2(0, -1)),
                rho_0(1), K(1), V(1), sigma(1), init_positions(nullptr),
                pressure_kernel(&(SmoothKernels::Poly6<D2>())),
                viscosity_kernel(&(SmoothKernels::DebrunSpiky<D2>())),
                surface_tension_kernel(&(SmoothKernels::Poly6<D2>()))
        {
            // default values;
        }
    };

    explicit Fluid2D(Fluid2DParameters &params);
    ~Fluid2D();

    // update
    void update();

private:
    // simulation params
    Fluid2DParameters params;

    // particles position
    std::vector<vec2> positions;
//     // used for buffer swapped
//     std::vector<vec2> back_positions;

    // particles velocity
    std::vector<vec2> velocities;

    // initialize
    void init();
    // render fluid
    void render();
};

#endif // FLUID_2D_H