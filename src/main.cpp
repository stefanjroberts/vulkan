
#include "custom_types.h"
#include <cstdio>
#include <cstring>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    printf("\n\033[31m[Vulkan Validation Layer]: \033[0m %s\n\n", callback_data->pMessage);
    return VK_FALSE;
}

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
    std::vector<char const *> instance_extensions = {"VK_EXT_debug_utils"};
    std::vector<char const *> instance_layers = {"VK_LAYER_KHRONOS_validation"};
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

  private:
    void create_instance()
    {
        u32 glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        for (i32 i = 0; i < glfw_extension_count; i++)
        {
            instance_extensions.push_back(glfw_extensions[i]);
        }

        u32 available_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions;
        available_extensions.resize(available_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());

        for (i32 i = 0; i < instance_extensions.size(); i++)
        {
            bool extension_available = false;
            for (i32 j = 0; j < available_extensions.size(); j++)
            {
                if (strcmp(instance_extensions[i], available_extensions[j].extensionName) == 0)
                {
                    extension_available = true;
                }
            }
            if (!extension_available)
            {
                printf("Extension Unavailable: %s\n", instance_extensions[i]);
            }
        }

        u32 available_layer_count = 0;
        vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers;
        available_layers.resize(available_layer_count);
        vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data());

        for (i32 i = 0; i < instance_layers.size(); i++)
        {
            bool layer_available = false;
            for (i32 j = 0; j < available_layers.size(); j++)
            {
                if (strcmp(instance_layers[i], available_layers[j].layerName) == 0)
                {
                    layer_available = true;
                }
            }
            if (!layer_available)
            {
                printf("Layer Unavailable: %s", instance_layers[i]);
            }
        }

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
        create_info.enabledExtensionCount = instance_extensions.size();
        create_info.ppEnabledExtensionNames = instance_extensions.data();
        create_info.enabledLayerCount = instance_layers.size();
        create_info.ppEnabledLayerNames = instance_layers.data();

        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        {
            printf("Failed to create vulkan instance\n");
        }
        return;
    }

    void create_debug_messenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;

        PFN_vkCreateDebugUtilsMessengerEXT messenger_create_function =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (messenger_create_function == 0)
        {
            printf("Unable to locate debug messenger create function\n");
            return;
        }
        if (messenger_create_function(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
        {
            printf("Failed to create a debug messenger\n");
        }
        return;
    }

  public:
    VulkanApp(Window *window)
    {
        create_instance();
        create_debug_messenger();
    }
    ~VulkanApp()
    {
PFN_vkDestroyDebugUtilsMessengerEXT messenger_destroy_function =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (messenger_destroy_function == 0)
        {
            printf("Unable to locate debug messenger create function\n");
        }
        messenger_destroy_function(instance, debug_messenger, nullptr);
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

    printf("Entering Main Loop\n");
    while (!glfwWindowShouldClose(glfw_window->get_glfw_window()))
    {
        glfwPollEvents();
        vulkan_app->render();
        break;
    }

    delete vulkan_app;
    delete glfw_window;
    return 0;
}
