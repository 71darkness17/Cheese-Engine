//#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <main.hpp>
//#include <types.hpp>
#include <graphicCore.hpp>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow * window = glfwCreateWindow(800, 600, "GraphicsTest", nullptr, nullptr);
    GraphicCore graphics(window);

    graphics.setCamera({400, 300}, 1.0f);
    graphics.startGraphicThread();
    graphics.addRectangle({100, 100}, 600, 400, {1.f, 0.f, 0.f});

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}