#include <GLFW/glfw3.h>
#include <windows.h>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
const size_t MAX_SAMPLES = 1000;

float history1[MAX_SAMPLES] = {0};
float history2[MAX_SAMPLES] = {0};
float history3[MAX_SAMPLES] = {0};
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

// Function to push data to the history arrays
void push_data(float x, float y, float z) {
    history1[head1] = x;
    head1 = (head1 + 1) % MAX_SAMPLES;

    history2[head2] = y;
    head2 = (head2 + 1) % MAX_SAMPLES;

    history3[head3] = z;
    head3 = (head3 + 1) % MAX_SAMPLES;
}

// Update amplitude range to track min and max values over all three histories
void updateAmplitudeRange() {
    minAmplitude = history1[0];
    maxAmplitude = history1[0];
    
    for (size_t i = 0; i < MAX_SAMPLES; ++i) {
        if (history1[i] < minAmplitude) minAmplitude = history1[i];
        if (history1[i] > maxAmplitude) maxAmplitude = history1[i];
        if (history2[i] < minAmplitude) minAmplitude = history2[i];
        if (history2[i] > maxAmplitude) maxAmplitude = history2[i];
        if (history3[i] < minAmplitude) minAmplitude = history3[i];
        if (history3[i] > maxAmplitude) maxAmplitude = history3[i];
    }
    
    // Ensure a margin to avoid clipping
    float margin = (maxAmplitude - minAmplitude) * 1.1f;
    minAmplitude -= margin;
    maxAmplitude += margin;
}

// Draw the data, normalizing y-coordinates based on min/max amplitude
void drawData(const float *history, size_t head, float offsetY, float aspectRatio, float r, float g, float b) {
    glColor3f(r, g, b); // Set the color
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < MAX_SAMPLES; ++i) {
        size_t index = (head + i) % MAX_SAMPLES;
        float x = (float)i / (float)(MAX_SAMPLES - 1) * 2.0f - 1.0f; // Normalize to [-1, 1]
        // Normalize to [-1, 1] based on min/max amplitude and add offset for multiple waves
        float y = ((history[index] - minAmplitude) / (maxAmplitude - minAmplitude)) * 2.0f - 1.0f; 
        y += offsetY; // Offset for multiple waves
        glVertex2f(x * aspectRatio, y);
    }
    glEnd();
}

int main() {
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

    framebuffer_size_callback(window, WIDTH, HEIGHT); // Set initial viewport and projection

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspectRatio = (float)width / (float)height;

        // Push random data for demonstration
        push_data(rand() % 100 / 100.0f, rand() % 100 / 100.0f, rand() % 100 / 100.0f);

        updateAmplitudeRange(); // Update the range of amplitudes for autoscaling

        drawData(history1, head1, 0.5f, aspectRatio, 1.0f, 0.0f, 0.0f); // Draw data 1 with red color
        drawData(history2, head2, 0.0f, aspectRatio, 0.0f, 1.0f, 0.0f); // Draw data 2 with green color
        drawData(history3, head3, -0.5f, aspectRatio, 0.0f, 0.0f, 1.0f); // Draw data 3 with blue color

        glfwSwapBuffers(window);
        glfwPollEvents();
        Sleep(10); // Sleep for 10 milliseconds
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
