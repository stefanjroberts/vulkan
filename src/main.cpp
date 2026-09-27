#include "core.h"
#include "custom_types.h"
#include "vulkanapp.h"
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

Input input = {};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        input.w = true;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
        input.w = false;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        input.s = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
        input.s = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        input.a = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        input.a = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        input.d = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        input.d = false;
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        input.left = true;
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
    {
        input.left = false;
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        input.right = true;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
    {
        input.right = false;
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        input.up = true;
    }
    if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
    {
        input.up = false;
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        input.down = true;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
    {
        input.down = false;
    }
}

int main()
{
    u32 width = 800;
    u32 height = 600;
    Window *glfw_window = new Window(width, height);
    glfwSetKeyCallback(glfw_window->get_glfw_window(), key_callback);

    VulkanApp *vulkan_app = new VulkanApp(glfw_window);

    glm::mat4 camera_position = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

    camera_position[1][2] = 1;

    printf("Entering Main Loop\n");
    while (!glfwWindowShouldClose(glfw_window->get_glfw_window()))
    {
        glfwPollEvents();

        vulkan_app->update_camera(&camera_position, input);
        vulkan_app->render(camera_position);
    }

    delete vulkan_app;
    delete glfw_window;
    return 0;
}
