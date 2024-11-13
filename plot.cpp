#include "plot.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <windows.h>

extern HANDLE data_ready;

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// Define initial buffer size
size_t bufferSize = 100;

std::vector<float> history1(bufferSize, 0.0f);
std::vector<float> history2(bufferSize, 0.0f);
std::vector<float> history3(bufferSize, 0.0f);
size_t head1 = 0;
size_t head2 = 0;
size_t head3 = 0;

float minAmplitude = 0.0f;
float maxAmplitude = 1.0f;

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
    const size_t maxBufferSize = 10000;

    if (yoffset > 0 && bufferSize < maxBufferSize) {
        bufferSize += 10;
    } else if (yoffset < 0 && bufferSize > minBufferSize) {
        bufferSize -= 10;
    }

    history1.resize(bufferSize, 0.0f);
    history2.resize(bufferSize, 0.0f);
    history3.resize(bufferSize, 0.0f);

    head1 = head1 % bufferSize;
    head2 = head2 % bufferSize;
    head3 = head3 % bufferSize;
}

// Function to push data to the history arrays
void push_data(float x, float y, float z) {
    history1[head1] = x;
    head1 = (head1 + 1) % bufferSize;

    history2[head2] = y;
    head2 = (head2 + 1) % bufferSize;

    history3[head3] = z;
    head3 = (head3 + 1) % bufferSize;
}

// Update amplitude range to track min and max values over all three histories
void updateAmplitudeRange() {
    minAmplitude = history1[0];
    maxAmplitude = history1[0];
    
    for (size_t i = 0; i < bufferSize; ++i) {
        if (history1[i] < minAmplitude) minAmplitude = history1[i];
        if (history1[i] > maxAmplitude) maxAmplitude = history1[i];
        if (history2[i] < minAmplitude) minAmplitude = history2[i];
        if (history2[i] > maxAmplitude) maxAmplitude = history2[i];
        if (history3[i] < minAmplitude) minAmplitude = history3[i];
        if (history3[i] > maxAmplitude) maxAmplitude = history3[i];
    }
    
    // Ensure a margin to avoid clipping
    float margin = (maxAmplitude - minAmplitude) * 0.1f;
    minAmplitude -= margin;
    maxAmplitude += margin;
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

int startOpenGL() {
    if (!glfwInit()) {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Scrolling Data with Autoscaling", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
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

        WaitForSingleObject(data_ready, 1);

        updateAmplitudeRange(); // Update the range of amplitudes for autoscaling

        drawData(history1, head1, 0.0f, aspectRatio, 1.0f, 0.0f, 0.0f); // Draw data 1 with red color
        drawData(history2, head2, 0.0f, aspectRatio, 0.0f, 1.0f, 0.0f); // Draw data 2 with green color
        drawData(history3, head3, 0.0f, aspectRatio, 0.0f, 0.0f, 1.0f); // Draw data 3 with blue color

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
