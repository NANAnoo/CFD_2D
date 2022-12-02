#ifndef GL_WINDOW_H
#define GL_WINDOW_H

/* project includes */
#include "GLHeaders.h"
#include "GLRenderable.h"
#include "Vec.h"

/* std includes */
#include <memory>
#include <vector>

class GLWindow
{
private:
// private properties

    /* windows width */
    unsigned int width;
    /* windows height*/
    unsigned int height;
    /* windows name */
    const char *w_name;
    /* glfw window */
    GLFWwindow* window;
    /* all render-able objects*/
    std::vector<std::shared_ptr<GLRenderableI> > render_objects;
    /* background color*/
    vec3 background_color;
    /* valid */
    bool valid;
    /* 1. /  desire FPS*/
    double frame_update_duration;
    // init object
    void init();

public:
    explicit GLWindow(unsigned int w = 800, unsigned int h = 800, const char *name = "new window"): w_name(name), width(w), height(h), window(nullptr), valid(false)
    {
        frame_update_duration = 1.0 / 60.0;
        init();
    };

    /* main loop */
    int run();
    /* main render */
    void render();
    /* add render object */
    void addRenderObject(std::shared_ptr<GLRenderableI> obj) {
        render_objects.push_back(obj);
    };
    /* setup background color */
    void setBackgroundColor(float r, float g, float b) {
        background_color.x() = r;
        background_color.y() = g;
        background_color.z() = b;
    }
    /* is valid */
    bool isValid() const
    {
        return valid;
    }
    /* update FPS */
    void updateFPS(unsigned int fps)
    {
        if (fps == 0) {
            fps = 60;
        }
        frame_update_duration = 1.0 / (double)fps;
    }
    /* set up size */
    void updateFrameSize(unsigned int w, unsigned int h);

    ~GLWindow();
};

#endif // GL_WINDOW_H

