#ifndef PLOT_H
#define PLOT_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstddef>
#include <vector>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void rescanAmplitudeRange();
void push_data(size_t num_vars, ...);
void drawData(const std::vector<float>& history, size_t head, float offsetY, float aspectRatio, float r, float g, float b);
void startOpenGL();

#endif // PLOT_H
