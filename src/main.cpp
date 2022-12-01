#include "GLWindow.h"
#include "GLRenderable.h"

int main(int, char **) {
    GLWindow window(800, 600);
    if (window.isValid()) {
        window.setBackgroundColor(0.2, 0.2, 0.3);
        return window.run();
    } else {
        std::cout << "Error occurs when window created" << std::endl;
    }
    return -1;
}