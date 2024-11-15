#include "plot.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <windows.h>
#include <algorithm>
#include <limits>
#include <cstdarg>
#include <atomic>
#include <mutex>

using namespace std;

#define MAX_BUFFER_SIZE 10000

extern HANDLE data_ready;

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
size_t bufferSize = 100;

// Pre-allocated buffers to store data history (Circular Buffers)
std::vector<std::vector<float>> histories;
std::vector<size_t> heads;

float currentMinAmplitude = std::numeric_limits<float>::max();
float currentMaxAmplitude = std::numeric_limits<float>::lowest();
float minAmplitude = 0.0f;
float maxAmplitude = 1.0f;
std::atomic<bool> requiresRescan(false);

const std::vector<std::array<float, 3>> colorSet = {
    {1.0f, 0.0f, 0.0f},   // Red
    {0.0f, 1.0f, 0.0f},   // Green
    {0.0f, 0.0f, 1.0f},   // Blue
    {1.0f, 1.0f, 0.0f},   // Yellow
    {0.0f, 1.0f, 1.0f},   // Cyan
    {1.0f, 0.0f, 1.0f},   // Magenta
    {0.5f, 0.5f, 0.5f},   // Gray
    {0.5f, 0.0f, 0.0f},   // Maroon
    {0.0f, 0.5f, 0.0f},   // Dark Green
    {0.0f, 0.0f, 0.5f},   // Navy
    {1.0f, 0.5f, 0.0f},   // Orange
    {0.5f, 0.0f, 0.5f},   // Purple
    {0.5f, 0.5f, 0.0f},   // Olive
    {0.0f, 0.5f, 0.5f},   // Teal
    {1.0f, 0.75f, 0.8f},  // Pink
    {0.75f, 1.0f, 0.75f}, // Light Green
    {0.75f, 0.75f, 1.0f}, // Light Blue
    {1.0f, 1.0f, 0.75f},  // Light Yellow
    {0.75f, 1.0f, 1.0f},  // Light Cyan
    {1.0f, 0.75f, 1.0f}   // Light Magenta
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspectRatio = (float)width / (float)height;
    if (aspectRatio > 1.0f) {
        glOrtho(-aspectRatio, aspectRatio, -1.0, 1.0, -1.0, 1.0);
    } else {
        glOrtho(-1.0, 1.0, -1.0 / aspectRatio, 1.0 / aspectRatio, -1.0, 1.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    const size_t minBufferSize = 10;
    const size_t maxBufferSize = MAX_BUFFER_SIZE;

    if (yoffset > 0 && bufferSize < maxBufferSize) {
        bufferSize += 10;
    } else if (yoffset < 0 && bufferSize > minBufferSize) {
        bufferSize -= 10;
    }

    for (auto& history : histories) {
        history.resize(bufferSize, 0.0f);
    }

    for (auto& head : heads) {
        head = head % bufferSize;
    }
}

// Function to scan all histories for the new min and max
void rescanAmplitudeRange() {
    currentMinAmplitude = std::numeric_limits<float>::max();
    currentMaxAmplitude = std::numeric_limits<float>::lowest();
    
    for (size_t i = 0; i < bufferSize; ++i) {
        for (size_t j = 0; j < histories.size(); ++j) {
            currentMinAmplitude = std::min(currentMinAmplitude, histories[j][i]);
            currentMaxAmplitude = std::max(currentMaxAmplitude, histories[j][i]);
        }
    }
    
    float margin = (currentMaxAmplitude - currentMinAmplitude) * 0.1f;
    minAmplitude = currentMinAmplitude - margin;
    maxAmplitude = currentMaxAmplitude + margin;
}

void push_data(size_t num_vars, ...) {
    va_list args;
    va_start(args, num_vars);

    // Resize histories and heads if needed
    if (histories.size() < num_vars) {
        histories.resize(num_vars, std::vector<float>(bufferSize, 0.0f));
        heads.resize(num_vars, 0);
    }

    // Push the new data
    for (size_t i = 0; i < num_vars; ++i) {
        float value = static_cast<float>(va_arg(args, double)); // Use double because va_arg promotes float to double

        // Check if the value to be overwritten is the current min or max
        if (histories[i][heads[i]] == currentMinAmplitude || histories[i][heads[i]] == currentMaxAmplitude) {
            requiresRescan = true;
        }

        histories[i][heads[i]] = value;
        heads[i] = (heads[i] + 1) % bufferSize;

        // Update running min/max
        if (!requiresRescan) {
            currentMinAmplitude = std::min(currentMinAmplitude, value);
            currentMaxAmplitude = std::max(currentMaxAmplitude, value);
        }
    }

    va_end(args);

    // Ensure a margin to avoid clipping
    float margin = (currentMaxAmplitude - currentMinAmplitude) * 0.1f;
    minAmplitude = currentMinAmplitude - margin;
    maxAmplitude = currentMaxAmplitude + margin;

    // Rescan if required
    if (requiresRescan) {
        rescanAmplitudeRange();
        requiresRescan = false;
    }
}

// Draw the data, normalizing y-coordinates based on min/max amplitude
void drawData(const std::vector<float> &history, size_t head, float offsetY, float aspectRatio, float r, float g, float b) {
    glColor3f(r, g, b); // Set the color
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < bufferSize; ++i) {
        size_t index = (head + i) % bufferSize;
        float x = (float)i / (float)(bufferSize - 1) * 2.0f - 1.0f; // Normalize to [-1, 1]
        // Normalize to [-1, 1] based on min/max amplitude and add offset for multiple waves
        float y = ((history[index] - minAmplitude) / (maxAmplitude - minAmplitude)) * 2.0f - 1.0f; 
        y += offsetY; // Offset for multiple waves
        glVertex2f(x * aspectRatio, y);
    }
    glEnd();
}

void startOpenGL() {
    // Initialize the data structures for plotting
    histories.resize(1, std::vector<float>(bufferSize, 0.0f));
    heads.resize(1, 0);

    if (!glfwInit()) {
        return;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Scrolling Data with Autoscaling", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback); // Set the scroll callback

    framebuffer_size_callback(window, WIDTH, HEIGHT); // Set initial viewport and projection

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspectRatio = (float)width / (float)height;

        WaitForSingleObject(data_ready, 10);

        // Draw each history with different colors
        for (size_t i = 0; i < histories.size(); ++i) {
            const auto& color = colorSet[i % colorSet.size()];
            drawData(histories[i], heads[i], 0.0f, aspectRatio, color[0], color[1], color[2]);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
