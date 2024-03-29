#ifndef FLUID_2D_H
#define FLUID_2D_H

#include "GLRenderable.h"
#include "Vec.h"
#include <vector>
#include <unordered_set>
#include <type_traits>
#include "SmoothKernels.h"
#include "ThreadPool.h"

class BoundaryI {
public:
    virtual void updateCS(float top, float bottom, float right, float left) = 0;
    // index : particle index
    // next_pos : check next position with the boundary
    // all_pos, all_vel : particles positions and velocities
    virtual bool updateAt(int index, vec2 next_pos, std::vector<vec2> &all_pos, std::vector<vec2> &all_vel) = 0;
    virtual bool isSeperated(vec2 a, vec2 b) = 0;
};

class Fluid2D final : public GLRenderableI {
public:
    struct Fluid2DParameters {
        // time step
        float delta_t;
        // boundary;
        float top;
        float bottom;
        float left;
        float right;
        // h in kernel function
        float h;
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
        void (*init_positions)(std::vector<vec2 > &positions, float top, float bottom, float left, float right);

        // smooth kernels
        SmoothKernels::SmoothKernel<D2> *rho_kernel;
        SmoothKernels::SmoothKernel<D2> *pressure_kernel;
        SmoothKernels::SmoothKernel<D2> *viscosity_kernel;
        SmoothKernels::SmoothKernel<D2> *surface_tension_kernel;

        Fluid2DParameters():
                top(1), bottom(-1), left(-1), right(1), h(1), delta_t(0.05),
                particle_count(1000), particle_mass(1), gravity(vec2(0, -1)),
                rho_0(1), K(1), V(1), sigma(1), init_positions(nullptr),
                rho_kernel(nullptr),
                pressure_kernel(nullptr),
                viscosity_kernel(nullptr),
                surface_tension_kernel(nullptr) {
            // default values;
        }
    };

    explicit Fluid2D(Fluid2DParameters &params);

    ~Fluid2D() final;

    // update
    void update() final;

    // start simulation
    void start();

    void stop() {
        is_running = false;
    }

    bool isRunning() const {
        return is_running;
    }

    void addBoundary(std::shared_ptr<BoundaryI> b) {
        b->updateCS(params.top, params.bottom, params.right, params.left);
        boundaries.push_back(b);
    }

    void resetWithCallback(std::function<void(void)> callback);

    // render scale
    void setScale(float s) {
        this->scale = s;
    }
    // simulation params
    Fluid2DParameters params;

private:
    // particles position
    std::vector<vec2 > positions;
    // used for buffer swapped
    std::vector<vec2 > back_positions;
    // particles velocity
    std::vector<vec2 > velocities;
    // current accelerations
    std::vector<vec2 > acc_s;
    // boundaries
    std::vector<std::shared_ptr<BoundaryI>> boundaries;

    // a grid used for acceleration
    std::vector<std::vector<int> > grid;
    int grid_raw;
    int grid_col;

    inline bool inGrid(int x, int y) const {
        return x < grid_col && x >= 0 && y < grid_raw && y >= 0;
    }

    inline std::vector<int> &cellAt(int x, int y) {
        return grid[y * grid_col + x];
    }

    // one simulation step
    void step();

    void index_all_particles();

    void acceleration(const std::vector<vec2 > &position,
                      const std::vector<vec2 > &velocity,
                      std::vector<vec2 > &acc);

    void acceleration_at(int p_index,
                         vec2 surf_n,
                         const std::vector<int> &neighbours,
                         const std::vector<vec2 > &position,
                         const std::vector<vec2 > &velocity,
                         const std::vector<float> &pho_s,
                         std::vector<vec2 > &acc);

    void update_boundary(int p_index, std::vector<vec2 > &position, std::vector<vec2 > &velocity) const;

    bool isSeperatedByBoundaries(int index1, int index2, const std::vector<vec2> &positions) {
        for (auto &b : boundaries) {
            if (b->isSeperated(positions[index1], positions[index2])) {
                return true;
            }
        }
        return false;
    }

    bool isSeperatedByBoundaries(vec2 v1, vec2 v2) {
        for (auto &b : boundaries) {
            if (b->isSeperated(v1, v2)) {
                return true;
            }
        }
        return false;
    }

    // thread
    nano_std::WorkerThread dispatcher;
    nano_std::ThreadPool *pool;
    std::mutex swap_mutex;
    bool is_running;

    // initialize
    void init();

    // render fluid
    void render();

    // render parameters
    float scale;
};

#endif // FLUID_2D_H