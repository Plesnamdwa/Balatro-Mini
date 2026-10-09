#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

int main() {
    if (!glfwInit()) {
        cerr << "Failed to init GLFW\n";
        return -1;
    }
    cout << "GLFW Initialized successfully!\n";
    glfwTerminate();
    return 0;
}
