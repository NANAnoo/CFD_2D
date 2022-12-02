#include "GLWindow.h"
#include "GLRenderable.h"
#include "SmoothKernels.h"

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
        float left = (*k)(p, 1);
        float _x = -1;
        for (unsigned int x = 1; x < 1000; x ++) {
            float _x_ = (float)x / 500.f  - 1.f;
            p = vec2( _x_, 0);
            float right = (*k)(p, 1);
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
    auto kernel = SmoothKernels::Viscosity<D2>();
    GLWindow window(800, 600, "SPH 2D");
    if (window.isValid()) {
        window.setBackgroundColor(0.2, 0.2, 0.3);
        auto k_ptr = std::make_shared<KernelViewer>(&kernel);
        window.addRenderObject(k_ptr);
        return window.run();
    } else {
        std::cout << "Error occurs when window created" << std::endl;
    }
    return 0;
}