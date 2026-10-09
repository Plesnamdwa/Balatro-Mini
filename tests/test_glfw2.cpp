#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* win = glfwCreateWindow(640, 480, "Test", NULL, NULL);
    glfwMakeContextCurrent(win);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, -1, 1);
    cout << "OpenGL 2D Ortho works perfectly with GLFW without glad!\n";
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
