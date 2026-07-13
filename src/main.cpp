
#include "custom_types.h"
#include <cstdio>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
  private:
    GLFWwindow *window;

  public:
    Window(u32 width, u32 height)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
    }

    ~Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow *get_glfw_window()
    {
        return window;
    }
};

class VulkanApp
{
  private:
    VkInstance instance;

  private:
    void create_instance()
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Vulkan Tutorial";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_4;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = 0;
        create_info.ppEnabledExtensionNames = nullptr;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;

        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        {
            printf("Failed to create vulkan instance\n");
        }
    }

  public:
    VulkanApp(Window *window)
    {
        create_instance();
    }
    ~VulkanApp()
    {
        vkDestroyInstance(instance, nullptr);
    }
    void render()
    {
        return;
    }
};

int main()
{
    u32 width = 800;
    u32 height = 600;
    Window *glfw_window = new Window(width, height);
    VulkanApp *vulkan_app = new VulkanApp(glfw_window);
    while (!glfwWindowShouldClose(glfw_window->get_glfw_window()))
    {
        glfwPollEvents();
        vulkan_app->render();
    }
    delete vulkan_app;
    delete glfw_window;
    return 0;
}
