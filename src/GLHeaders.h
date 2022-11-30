#ifndef GL_HEADERS_H
#define GL_HEADERS_H

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#endif

#include <iostream>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

#endif // gl hearders