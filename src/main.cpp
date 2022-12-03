#include "GLWindow.h"
#include "GLRenderable.h"
#include "SmoothKernels.h"
#include "Fluid2D.h"
#include <chrono>
#include <random>

using namespace std;

struct KernelViewer final : public GLRenderableI
{
    SmoothKernels::SmoothKernel<D2> *k;
public:
    explicit KernelViewer(SmoothKernels::SmoothKernel<D2> *kernel) {
        k = kernel;
    }
    ~KernelViewer() final = default;
    void update () final
    {
        glLoadIdentity();
        glScalef(0.75, 0.05, 1);
        glColor3f(1, 0, 0);
        glLineWidth(2);
        glBegin(GL_LINES);
        vec2 p(- 1.f, 0);
        float left = (*k)(p, 0.1);
        float _x = -1;
        for (unsigned int x = 1; x < 1000; x ++) {
            float _x_ = (float)x / 500.f  - 1.f;
            p = vec2( _x_, 0);
            float right = (*k)(p, 0.1);
            glVertex3f(_x, left, 0);
            glVertex3f(_x_, right, 0);
            left = right;
            _x = _x_;
        }
        glEnd();

        glColor3f(0, 1, 0);
        glLineWidth(2);
        glBegin(GL_LINES);
        p = vec2(- 1.f, 0);
        left = (*k).diff(p, 1).x();
        _x = -1;
        for (unsigned int x = 1; x < 1000; x ++) {
            float _x_ = (float)x / 500.f  - 1.f;
            p = vec2( _x_, 0);
            float right = (*k).diff(p, 1).x();
            glVertex3f(_x, left, 0);
            glVertex3f(_x_, right, 0);
            left = right;
            _x = _x_;
        }
        glEnd();

        glColor3f(0, 0, 1);
        glLineWidth(2);
        glBegin(GL_LINES);
        p = vec2(- 1.f, 0);
        left = (*k).laplace(p, 1);
        _x = -1;
        for (unsigned int x = 1; x < 1000; x ++) {
            float _x_ = (float)x / 500.f  - 1.f;
            p = vec2( _x_, 0);
            float right = (*k).laplace(p, 1);
            glVertex3f(_x, left, 0);
            glVertex3f(_x_, right, 0);
            left = right;
            _x = _x_;
        }
        glEnd();

        glColor3f(0.2, 0.5, 0.7);
        glLineWidth(1);
        glBegin(GL_LINES);
        glVertex3f(-1, 0, 0);
        glVertex3f(1, 0, 0);
        glVertex3f(0, -1, 0);
        glVertex3f(0, 1, 0);
        glEnd();
    }
};

int main(int, char **) {
    int width = 800;
    int height = 500;
    // simulate parameters
    Fluid2D::Fluid2DParameters params;
    params.delta_t = 0.05;
    params.top = height / 10.f;
    params.bottom = 0;
    params.left = 0;
    params.right = width / 10.f;
    params.rho_0 = 10;
    params.K = 1;
    params.V = 0.3;
    params.sigma = 0.1;
    params.particle_count = 5000;
    params.gravity = vec2(0, -1);
    params.rho_kernel = &SmoothKernels::Poly6<D2>();
    params.pressure_kernel = &SmoothKernels::DebrunSpiky<D2>();
    params.viscosity_kernel = &SmoothKernels::Viscosity<D2>();
    params.surface_tension_kernel = &SmoothKernels::Poly6<D2>();
    params.h = 1;
    params.init_positions = [](std::vector<Vec<D2>> &positions,float t,float b,float l,float r) {
        float unit_size = std::min((r - l), (t - b)) / 10;
        for (auto &p : positions) {
            p.x() = unit_size * ((float(std::rand()) / float(RAND_MAX) * 4 + 3)) + l;
            p.y() = unit_size * ((float(std::rand()) / float(RAND_MAX) * 6 + 3.5)) + b;
        }
    };

    // init fluid
    auto fluid = std::make_shared<Fluid2D>(params);
    fluid->setScale(20.f / float(std::min(width, height)));

    // build window
    GLWindow window(800, 500, "SPH 2D");
    window.updateFPS(120);
    if (window.isValid()) {
        window.setBackgroundColor(0.1, 0.0, 0.1);
        window.addRenderObject(fluid);
        fluid->start();
        return window.run();
    } else {
        std::cout << "Error occurs when window created" << std::endl;
    }
    return 0;
}