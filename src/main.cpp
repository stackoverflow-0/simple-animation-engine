#include "mesh.hpp"
#include <GLFW/glfw3.h>
#include <stdio.h>

static GLFWwindow *window = NULL;
static int swapAndPollInput();

void display()
{

    // Set every pixel in the frame buffer to the current clear color.
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing is done by specifying a sequence of vertices.  The way these
    // vertices are connected (or not connected) depends on the argument to
    // glBegin.  GL_POLYGON constructs a filled polygon.
    glBegin(GL_POLYGON);
    glColor3f(1, 0, 0);
    glVertex3f(-0.6, -0.75, 0.5);
    glColor3f(0, 1, 0);
    glVertex3f(0.6, -0.75, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0.75, 0);
    glEnd();

    // Flush drawing command buffer to make drawing happen as soon as possible.
    glFlush();
}

static void run()
{
    glClearColor(0.6f, 0, 0.3f, 0);
    do
    {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
    } while (swapAndPollInput());
}

static int swapAndPollInput()
{
    glfwSwapBuffers(window);
    glfwPollEvents();
    return !glfwWindowShouldClose(window);
}

static void glfwErrorCallback(int, const char *message)
{
    fprintf(stderr, "%s\n", message);
}

int main()
{
    // GLFW + OpenGL init
    glfwInit();
    glfwSetErrorCallback(glfwErrorCallback);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
    window = glfwCreateWindow(1024, 768, "GL test app", NULL, NULL);
    glfwMakeContextCurrent(window);

    model::Model human_with_skeleton{};

    human_with_skeleton.load_model("asset/alliance.obj");

    // Event loop
    run();
    // Cleanup
    glfwTerminate();
    return 0;
}