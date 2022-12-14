#include "GLWindow.h"
#include "GLRenderable.h"
#include "Fluid2D.h"
#include "LineBoundary.h"
#include <random>

// define H to use default IMPL
#ifndef KERNEL_WITH_H
#define KERNEL_WITH_H 1.f
#include "SmoothKernels.h"
#endif

using namespace std;
const float axis_short_size = 8;

// window size
const int width = 800;
const int height = 800;

// init location of the liquid
const float init_x = 2;
const float init_y = 4;
const float init_w = 2;
const float init_h = 3;

// grid info
const float grid_size = 10;
// particle count of each unit area
const int p_cnt_per_u = int(16 * 10000 / grid_size / grid_size);

// simulation params
const float rho_0 = 18;
const float K = 1;
const float miu = 0.3;
const float sigma = 0.05;
const vec2 G(0, -0.5);
const float dt = 0.05;

struct coords_painter final : public GLRenderableI {
    float width;
    float height;
    float unit_size;

    explicit coords_painter(float w, float h, float u)
            : width(w), height(h), unit_size(u) {}

    void update() final {
        glColor3f(0.1, 0.3, 0.1);
        glLineWidth(1);
        int v_lines = std::ceil(height / unit_size);
        int h_lines = std::ceil(width / unit_size);
        float cell = 2 * unit_size / std::min(width, height);
        glBegin(GL_LINES);
        for (int v = 0; v <= v_lines; v++) {
            glVertex3f(-1, cell * v - 1, 0);
            glVertex3f(1, cell * v - 1, 0);
        }
        for (int h = 0; h <= h_lines; h++) {
            glVertex3f(cell * h - 1, -1, 0);
            glVertex3f(cell * h - 1, 1, 0);
        }
        glEnd();

    }

    ~coords_painter() override = default;
};

Fluid2D::Fluid2DParameters basic_params() {
    Fluid2D::Fluid2DParameters params;
    params.delta_t = dt;
    params.top = height / grid_size;
    params.bottom = 0;
    params.left = 0;
    params.right = width / grid_size;
    params.rho_0 = rho_0;
    params.K = K;
    params.V = miu;
    params.sigma = sigma;
    params.particle_count = int(p_cnt_per_u * init_w * init_h);
    params.gravity = G;
    params.rho_kernel = &Poly6<D2>();
    params.pressure_kernel = &DebrunSpiky<D2>();
    params.viscosity_kernel = &Viscosity<D2>();
    params.surface_tension_kernel = &Poly6<D2>();
    params.h = H;
    params.init_positions = [](std::vector<Vec<D2>> &positions, float t, float b, float l, float r) {
        float unit_size = std::min((r - l), (t - b)) / axis_short_size;
        float unit_count = std::sqrt(float(positions.size()) / (init_w * init_h));
        float step_size = 1.f / unit_count;
        int width = std::ceil(unit_count * init_w), height = std::ceil(unit_count * init_h);
        for (int dx = 0; dx < width; dx++) {
            for (int dy = 0; dy < height; dy++) {
                int index = dy * width + dx;
                if (index >= positions.size()) return;
                positions[index].x() = l + unit_size * (float(dx) * step_size + 0.5 * step_size + init_x);
                positions[index].y() = b + unit_size * (float(dy) * step_size + 0.5 * step_size + init_y);
            }
        }
//        for (auto &p: positions) {
//            p.x() = unit_size * ((float(std::rand()) / float(RAND_MAX) * init_w + init_x)) + l;
//            p.y() = unit_size * ((float(std::rand()) / float(RAND_MAX) * init_h + init_y)) + b;
//        }
    };
    return params;
}

// a handler used for switching case
class EventHandler final : public GLWindowEventHandler {
private:
    std::shared_ptr<Fluid2D> f;
public:
    explicit EventHandler(std::shared_ptr<Fluid2D> &fluid) : f(fluid) {}

    void keyDown(GLWindow *window, int key) override {
        if (key == GLFW_KEY_SPACE) {
            if (f->isRunning()) {
                f->stop();
            } else {
                f->start();
            }
        } else if (key == GLFW_KEY_1) {
            f->resetWithCallback([this]() {
                if (!f->isRunning()) {
                    //f->params.delta_t = 0.01;
                    f->params.pressure_kernel = &Poly6<D2>();
                    f->params.viscosity_kernel = nullptr;
                    f->params.surface_tension_kernel = nullptr;
                    f->params.K = 0.2;
                    f->params.V = 0;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_2) {
            f->resetWithCallback([this]() {
                if (!f->isRunning()) {
                    //f->params.delta_t = 0.01;
                    f->params.pressure_kernel = &DebrunSpiky<D2>();
                    f->params.viscosity_kernel = nullptr;
                    f->params.surface_tension_kernel = nullptr;
                    f->params.K = 0.2;
                    f->params.V = 0;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_3) {
            f->resetWithCallback([this]() {
                if (!f->isRunning()) {
                    f->params.delta_t = dt;
                    f->params.pressure_kernel = &DebrunSpiky<D2>();
                    f->params.viscosity_kernel = &Viscosity<D2>();
                    f->params.surface_tension_kernel = nullptr;
                    f->params.K = K;
                    f->params.V = miu;
                    f->params.sigma = 0;
                }
            });
        } else if (key == GLFW_KEY_4) {
            f->resetWithCallback([this]() {
                if (!f->isRunning()) {
                    f->params.delta_t = dt;
                    f->params.pressure_kernel = &DebrunSpiky<D2>();
                    f->params.viscosity_kernel = &Viscosity<D2>();
                    f->params.surface_tension_kernel = &Poly6<D2>();
                    f->params.K = K;
                    f->params.V = miu;
                    f->params.sigma = sigma;
                }
            });
        }
    }

    void keyUp(GLWindow *window, int key) override {
        // do nothing
    }
};

int main(int, char **) {
    float unit_size = float(std::min(width, height)) / axis_short_size;

    // my fluid
    Fluid2D::Fluid2DParameters params = basic_params();
    // init fluid
    auto fluid = std::make_shared<Fluid2D>(params);
    fluid->setScale(2.f * grid_size / float(std::min(width, height)));

    // other objects
    auto coords = std::make_shared<coords_painter>(width, height, unit_size);

    // walls
    float i = 1.f / axis_short_size;
    float damp = 0.1;
    std::shared_ptr<LineBoundary> wall_bottom = std::make_shared<LineBoundary>(i * 1, 0.0001, i * 6, 0.0001, damp);
    std::shared_ptr<LineBoundary> wall_left = std::make_shared<LineBoundary>(i * 1, 0.0001, i * 1, i * 5, damp);
    std::shared_ptr<LineBoundary> wall_right = std::make_shared<LineBoundary>(i * 6, 0.0001, i * 6, i * 5, damp);
//    std::shared_ptr<LineBoundary> wall = std::make_shared<LineBoundary>(i * 2.5, i * 3, i * 3, i * 3.5, damp);
//    std::shared_ptr<LineBoundary> wall2 = std::make_shared<LineBoundary>(i * 3, i * 3.5, i * 3.5, i * 3, damp);
    fluid->addBoundary(wall_left);
    fluid->addBoundary(wall_right);
    fluid->addBoundary(wall_bottom);
//    fluid->addBoundary(wall);
//    fluid->addBoundary(wall2);

    // build window
    GLWindow window(width, height, "SPH 2D");
    // event handler
    std::shared_ptr<EventHandler> handler = std::make_shared<EventHandler>(fluid);
    if (window.isValid()) {
        window.setBackgroundColor(0.1, 0.05, 0.1);
        window.addRenderObject(coords);
        window.addRenderObject(fluid);
        window.addRenderObject(wall_left);
        window.addRenderObject(wall_right);
        window.addRenderObject(wall_bottom);
//        window.addRenderObject(wall);
//        window.addRenderObject(wall2);
        window.updateFPS(120);
        window.delegate = handler;
        return window.run();
    } else {
        std::cout << "Error occurs when window created" << std::endl;
    }
    return 0;
}