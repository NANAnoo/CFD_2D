#include "GLWindow.h"
#include "GLRenderable.h"
#include "SmoothKernels.h"
#include "Fluid2D.h"
#include <chrono>
#include <random>

using namespace std;

const float axis_short_size = 8;

struct coords_painter final : public GLRenderableI {
    float width;
    float height;
    float unit_size;
    explicit coords_painter(float w, float h, float u)
    :width(w), height(h), unit_size(u){}
    void update() final {
        glColor3f(0.1, 0.3, 0.1);
        glLineWidth(1);
        int v_lines = std::ceil(height / unit_size);
        int h_lines = std::ceil(width / unit_size);
        float cell = 2 * unit_size / std::min(width, height);
        glBegin(GL_LINES);
        for (int v = 0; v <= v_lines; v ++) {
            glVertex3f(-1, cell * v - 1, 0);
            glVertex3f(1, cell * v - 1, 0);
        }
        for (int h = 0; h <= h_lines; h ++) {
            glVertex3f(cell * h - 1, - 1, 0);
            glVertex3f(cell * h - 1,  1, 0);
        }
        glEnd();

    }
    ~coords_painter() override = default;
};

Fluid2D::Fluid2DParameters basic_params(int width, int height, float grid_size)
{
    Fluid2D::Fluid2DParameters params;
    params.delta_t = 0.05;
    params.top = height / grid_size;
    params.bottom = 0;
    params.left = 0;
    params.right = width / grid_size;
    params.rho_0 = 6.5;
    params.K = 1;
    params.V = 0.3;
    params.sigma = 0.1;
    params.particle_count = 10000;
    params.gravity = vec2(0, -1);
    params.rho_kernel = &SmoothKernels::Poly6<D2>();
    params.pressure_kernel = &SmoothKernels::DebrunSpiky<D2>();
    params.viscosity_kernel = &SmoothKernels::Viscosity<D2>();
    params.surface_tension_kernel = &SmoothKernels::Poly6<D2>();
    params.h = 1;
    params.init_positions = [](std::vector<Vec<D2>> &positions,float t,float b,float l,float r) {
        float unit_size = std::min((r - l), (t - b)) / axis_short_size;
        for (auto &p : positions) {
            p.x() = unit_size * ((float(std::rand()) / float(RAND_MAX) * 2 + 2)) + l;
            p.y() = unit_size * ((float(std::rand()) / float(RAND_MAX) * 3 + 4)) + b;
        }
    };
    return params;
}

std::shared_ptr<Fluid2D> getFluid(int width, int height, float grid_size)
{
    Fluid2D::Fluid2DParameters params = basic_params(width, height, grid_size);
    // init fluid
    auto fluid = std::make_shared<Fluid2D>(params);
    fluid->setScale(2.f * grid_size / float(std::min(width, height)));
    // add walls
    float unit_size = float(std::min(width, height)) / axis_short_size;
    int cell_per_unit = int(unit_size / grid_size);
    auto one_h_wall = [&](float start, float end, float y) {
        int cy = y * cell_per_unit;
        int begin_x = start * cell_per_unit;
        int end_x = end * cell_per_unit;
        for (int x = begin_x; x <= end_x; x ++) {
            fluid->addWall(x, cy);
        }
    };
    auto one_v_wall = [&](float start, float end, float x) {
        int cx = x * cell_per_unit;
        int begin_y = start * cell_per_unit;
        int end_y = end * cell_per_unit;
        for (int y = begin_y; y <= end_y; y ++) {
            fluid->addWall(cx, y);
        }
    };
    // add a container
    one_h_wall(1, 6, 0);
    one_v_wall(0, 5, 1);
    one_v_wall(0, 5, 6);
    return fluid;
}


// a handler used for switching case
class EventHandler final : public GLWindowEventHandler
{
private:
    std::shared_ptr<Fluid2D> f;
public:
    explicit EventHandler(std::shared_ptr<Fluid2D> &fluid):f(fluid){}
    void keyDown(GLWindow *window, int key) override {
        if (key == GLFW_KEY_SPACE) {
            if (f->isRunning()) {
                f->stop();
            } else {
                f->start();
            }
        } else if (key == GLFW_KEY_1) {
            f->resetWithCallback([this](){
                if (!f->isRunning()) {
                    f->params.pressure_kernel = &SmoothKernels::Poly6<D2>();
                    f->params.viscosity_kernel = nullptr;
                    f->params.surface_tension_kernel = nullptr;
                    f->params.rho_0 = 10;
                    f->params.V = 0;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_2) {
            f->resetWithCallback([this](){
                if (!f->isRunning()) {
                    f->params.pressure_kernel = &SmoothKernels::DebrunSpiky<D2>();
                    f->params.viscosity_kernel = nullptr;
                    f->params.surface_tension_kernel = nullptr;
                    f->params.rho_0 = 10;
                    f->params.V = 0;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_3) {
            f->resetWithCallback([this](){
                if (!f->isRunning()) {
                    f->params.pressure_kernel = &SmoothKernels::DebrunSpiky<D2>();
                    f->params.viscosity_kernel = &SmoothKernels::Viscosity<D2>();
                    f->params.surface_tension_kernel = nullptr;
                    f->params.rho_0 = 6.5;
                    f->params.V = 0.3;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_4) {
            f->resetWithCallback([this](){
                if (!f->isRunning()) {
                    f->params.pressure_kernel = &SmoothKernels::DebrunSpiky<D2>();
                    f->params.viscosity_kernel = &SmoothKernels::Viscosity<D2>();
                    f->params.surface_tension_kernel = &SmoothKernels::Poly6<D2>();
                    f->params.rho_0 = 6.5;
                    f->params.V = 0.3;
                    f->params.sigma = 0.1;
                }
            });
        }
    }
    void keyUp(GLWindow *window, int key) override {
        // do nothing
    }
};

int main(int, char **) {
    int width = 768;
    int height = 768;
    float grid_size = 8;
    float unit_size = float(std::min(width, height)) / axis_short_size;

    // my fluid
    auto fluid = getFluid(width, height, grid_size);

    // other objects
    auto coords = std::make_shared<coords_painter>(width, height, unit_size);

    // build window
    GLWindow window(width, height, "SPH 2D");
    // event handler
    std::shared_ptr<EventHandler> handler = std::make_shared<EventHandler>(fluid);
    if (window.isValid()) {
        window.setBackgroundColor(0.1, 0.0, 0.1);
        window.addRenderObject(fluid);
        window.addRenderObject(coords);
        window.updateFPS(120);
        window.delegate = handler;
        return window.run();
    } else {
        std::cout << "Error occurs when window created" << std::endl;
    }
    return 0;
}