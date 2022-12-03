#include "GLWindow.h"

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>

// pass callback from glfw window to GLWindow
static std::unordered_map<GLFWwindow *, GLWindow *> all_windows;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    if (all_windows.find(window) != all_windows.end()) {
        GLWindow *ptr = all_windows[window];
        ptr->updateFrameSize(width, height);
    }
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (all_windows.find(window) != all_windows.end()) {
        GLWindow *ptr = all_windows[window];
        auto handler = ptr->delegate.lock();
        if (ptr->delegate.expired()) {
            return;
        }
        if (action == GLFW_PRESS) {
            handler->keyDown(ptr, key);
        } else if (action == GLFW_RELEASE) {
            handler->keyUp(ptr, key);
        }
    }
}

GLWindow::~GLWindow() {
    if (valid && window != nullptr) {
        all_windows.erase(window);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    std::cout << "GLWindow released" << std::endl;
}

void GLWindow::init() {
    // initial glfw
    valid = true;
    if (!glfwInit()) {
        std::cout << "glfw init failed" << std::endl;
        valid = false;
    }
    // initial window
    this->window = glfwCreateWindow(width, height, w_name, nullptr, nullptr);
    if (!window) {
        std::cout << "glfw window create failed" << std::endl;
        valid = false;
    }
    if (valid) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        // set up callbacks
        glfwSetErrorCallback(error_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetKeyCallback(window, keyboard_callback);
        valid = true;
        all_windows[window] = this;
    }
}

int GLWindow::run() {
    float aspect = (float) width / (float) height;
    // set aspect
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (aspect > 1) {
        glOrtho(-aspect, aspect, -1, 1, -1, 1);
    } else {
        glOrtho(-1, 1, -1.0 / aspect, 1.0 / aspect, -1, 1);
    }
    std::chrono::steady_clock::time_point t_start, t_end;
    static std::chrono::steady_clock::time_point last_time, cur_time;
    last_time = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        t_start = std::chrono::high_resolution_clock::now();
        render();
        GLenum e = glGetError();
        if (e > 0) {
            std::cout << "opengl error " << e << std::endl;
            return e;
        }
        t_end = std::chrono::high_resolution_clock::now();
        double duration = (t_end - t_start).count() * 1E-9;
        if (duration < frame_update_duration) {
            duration = frame_update_duration - duration;
            std::this_thread::sleep_for(std::chrono::seconds() * duration);
        }
        cur_time = std::chrono::high_resolution_clock::now();
        double frame_time = (cur_time - last_time).count() * 1E-9;
        last_time = cur_time;
        std::cout << "FPS : " << 1.0 / frame_time << std::endl;
    }
    return 0;
}

void GLWindow::render() {
    glClearColor(background_color.x(), background_color.y(), background_color.z(), 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // update all objects
    for (auto obj: render_objects) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        obj->update();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void GLWindow::updateFrameSize(unsigned int w, unsigned int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    // update aspect
    float aspect = (float) w / (float) h;

    // set projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (aspect > 1) {
        glOrtho(-aspect, aspect, -1, 1, -1, 1);
    } else {
        glOrtho(-1, 1, -1.0 / aspect, 1.0 / aspect, -1, 1);
    }
    render();
}