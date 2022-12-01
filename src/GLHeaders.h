#ifndef GL_HEADERS_H
#define GL_HEADERS_H

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#endif

#include <iostream>

// callback used in glfw windows
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error[%d]: %s\n", error, description);
}

#endif // GL_HEADERS_H