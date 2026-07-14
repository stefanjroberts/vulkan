
#include "custom_types.h"
#include <cstdio>
#include <cstring>
#include <vector>
#define VK_USE_PLATFORM_WAYLAND
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3native.h>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("\033[31m[Vulkan Validation Layer]: \033[0m %s\n", callback_data->pMessage);
    }

    else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        printf("\033[93m[Vulkan Validation Layer]: \033[0m %s\n", callback_data->pMessage);
    }

    else
    {
        printf("\033[96m[Vulkan Validation Layer]: \033[0m %s\n", callback_data->pMessage);
    }

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
    struct QueueFamilyInfo
    {
        std::vector<VkQueueFamilyProperties> families;
        u32 graphics_index;
        VkQueue graphics_queue;
        u32 presentation_index;
        VkQueue presentation_queue;
    };

    struct SwapchainInfo
    {
        VkSurfaceFormatKHR surface_format;
        VkExtent2D extent;
    };

  private:
    std::vector<char const *> instance_extensions = {"VK_EXT_debug_utils"};
    std::vector<char const *> instance_layers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> device_extensions = {"VK_KHR_swapchain", "VK_EXT_extended_dynamic_state"};

    VkInstance instance = 0;
    VkDebugUtilsMessengerEXT debug_messenger = 0;
    VkPhysicalDevice physical_device = 0;
    QueueFamilyInfo queue_family_info = {};
    VkDevice device = 0;
    VkSurfaceKHR surface = 0;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    SwapchainInfo swapchain_info;
    std::vector<VkImageView> swapchain_image_views;

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

    void create_surface(GLFWwindow *window)
    {
        VkWaylandSurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        surface_create_info.display = glfwGetWaylandDisplay();
        surface_create_info.surface = glfwGetWaylandWindow(window);

        if (vkCreateWaylandSurfaceKHR(instance, &surface_create_info, nullptr, &surface) != VK_SUCCESS)
        {
            printf("Failed to create wayland surface");
        }

        return;
    }

    void pick_physical_device()
    {
        u32 physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        if (physical_device_count == 0)
        {
            printf("No physical devices with vulkan support found");
        }
        std::vector<VkPhysicalDevice> candidate_devices;
        candidate_devices.resize(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, candidate_devices.data());

        std::vector<bool> device_rank; // Ordinarily this would be an integer, in our case the GPU is either suitable or it isn't.
        for (i32 i = 0; i < physical_device_count; i++)
        {
            device_rank.push_back(true);
        }
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT EDS_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
        VkPhysicalDeviceVulkan13Features vulkan_13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &EDS_features};
        VkPhysicalDeviceVulkan11Features vulkan_11_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &vulkan_13_features};
        VkPhysicalDeviceFeatures2 candidate_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &vulkan_11_features};
        VkPhysicalDeviceProperties2 candidate_properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        for (i32 i = 0; i < physical_device_count; i++)
        {
            vkGetPhysicalDeviceFeatures2(candidate_devices[i], &candidate_features);
            vkGetPhysicalDeviceProperties2(candidate_devices[i], &candidate_properties);
            u32 candidate_family_count;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate_devices[i], &candidate_family_count, nullptr);
            std::vector<VkQueueFamilyProperties> candidate_queue_families;
            candidate_queue_families.resize(candidate_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate_devices[i], &candidate_family_count, candidate_queue_families.data());

            if (candidate_properties.properties.apiVersion < VK_API_VERSION_1_3)
            {
                device_rank[i] = false;
            }

            bool graphics_supported = false;
            for (i32 j = 0; j < candidate_family_count; j++)
            {
                if (candidate_queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_supported = true;
                }
            }
            if (!graphics_supported)
            {
                device_rank[i] = false;
            }
            if (!vulkan_11_features.shaderDrawParameters)
            {
                device_rank[i] = false;
            }
            if (!vulkan_13_features.dynamicRendering)
            {
                device_rank[i] = false;
            }
            if (!EDS_features.extendedDynamicState)
            {
                device_rank[i] = false;
            }
        }

        for (i32 i = 0; i < physical_device_count; i++)
        {
            if (device_rank[i])
            {
                physical_device = candidate_devices[i];
                printf("Selecting physical device %d\n", i);
                break;
            }
        }
        if (physical_device == 0)
        {
            printf("No suitable physical device found");
        }
        return;
    }

    void create_device()
    {
        u32 queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        queue_family_info.families.resize(queue_family_count);

        for (i32 i = 0; i < queue_family_info.families.size(); i++)
        {
            if (queue_family_info.families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queue_family_info.graphics_index = i;
                break;
            }
        }
        for (i32 i = 0; i < queue_family_info.families.size(); i++)
        {
            if (vkGetPhysicalDeviceWaylandPresentationSupportKHR(physical_device, i, glfwGetWaylandDisplay()) == VK_TRUE)
            {
                queue_family_info.presentation_index = i;
                break;
            }
        }

        if (queue_family_info.graphics_index != queue_family_info.presentation_index)
        {
            printf("Graphics and presentation queues are different\n");
        }

        VkDeviceQueueCreateInfo queue_create_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        f32 queue_priority = 0.5f;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_info.queueFamilyIndex = queue_family_info.graphics_index;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT EDS_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};
        VkPhysicalDeviceVulkan13Features vulkan_13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, &EDS_features};
        VkPhysicalDeviceVulkan11Features vulkan_11_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &vulkan_13_features};
        VkPhysicalDeviceFeatures2 physical_device_features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &vulkan_11_features};

        vulkan_11_features.shaderDrawParameters = true;
        vulkan_13_features.dynamicRendering = true;
        EDS_features.extendedDynamicState = true;

        VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_create_info.pNext = &physical_device_features;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledExtensionCount = device_extensions.size();
        device_create_info.ppEnabledExtensionNames = device_extensions.data();

        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        {
            printf("Unable to create a logical device\n");
        }

        VkDeviceQueueInfo2 queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
        queue_info.queueFamilyIndex = queue_family_info.graphics_index;
        queue_info.queueIndex = 0;

        vkGetDeviceQueue2(device, &queue_info, &queue_family_info.graphics_queue);

        queue_info.queueFamilyIndex = queue_family_info.presentation_index;
        queue_info.queueIndex = 0;

        vkGetDeviceQueue2(device, &queue_info, &queue_family_info.presentation_queue);

        return;
    }

    void create_swapchain(GLFWwindow *window)
    {
        VkSurfaceCapabilitiesKHR surface_capabilities = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

        u32 surface_format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> surface_formats;
        surface_formats.resize(surface_format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());

        u32 present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        std::vector<VkPresentModeKHR> present_modes;
        present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());

        VkSurfaceFormatKHR surface_format = surface_formats[0];
        for (i32 i = 0; i < surface_formats.size(); i++)
        {
            if ((surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
            {
                surface_format = surface_formats[i];
            }
        }

        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (i32 i = 0; i < present_modes.size(); i++)
        {
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = present_modes[i];
            }
        }

        VkExtent2D swap_extent;

        i32 height, width;
        glfwGetFramebufferSize(window, &width, &height);
        swap_extent.height = height;
        swap_extent.width = width;
        if (swap_extent.height < surface_capabilities.minImageExtent.height)
        {
            swap_extent.height = surface_capabilities.minImageExtent.height;
        }
        if (swap_extent.width < surface_capabilities.minImageExtent.width)
        {
            swap_extent.width = surface_capabilities.minImageExtent.width;
        }
        if (swap_extent.height > surface_capabilities.maxImageExtent.height)
        {
            swap_extent.height = surface_capabilities.maxImageExtent.height;
        }
        if (swap_extent.width > surface_capabilities.maxImageExtent.width)
        {
            swap_extent.width = surface_capabilities.maxImageExtent.width;
        }

        u32 image_count = surface_capabilities.minImageCount + 1;
        if (image_count > surface_capabilities.maxImageCount)
        {
            image_count = surface_capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchain_create_info.surface = surface;
        swapchain_create_info.minImageCount = image_count;
        swapchain_create_info.imageFormat = surface_format.format;
        swapchain_create_info.imageColorSpace = surface_format.colorSpace;
        swapchain_create_info.imageExtent = swap_extent;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.preTransform = surface_capabilities.currentTransform;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = present_mode;
        swapchain_create_info.clipped = true;

        vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);
        u32 swapchain_image_count;
        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
        swapchain_images.resize(swapchain_image_count);
        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

        swapchain_info.extent = swap_extent;
        swapchain_info.surface_format = surface_format;
    }

    void create_image_views()
    {
        swapchain_image_views.resize(swapchain_images.size());
        VkImageViewCreateInfo IVC_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        IVC_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        IVC_info.format = swapchain_info.surface_format.format;
        IVC_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        IVC_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY};
        IVC_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        for (i32 i = 0; i < swapchain_images.size(); i++)
        {
            IVC_info.image = swapchain_images[i];
            vkCreateImageView(device, &IVC_info, nullptr, &swapchain_image_views[i]);
        }
    }

  public:
    VulkanApp(Window *window)
    {
        create_instance();
        create_debug_messenger();
        create_surface(window->get_glfw_window());
        pick_physical_device();
        create_device();
        create_swapchain(window->get_glfw_window());
        create_image_views();
    }
    ~VulkanApp()
    {
        for (i32 i = 0; i < swapchain_image_views.size(); i++)
        {
            vkDestroyImageView(device, swapchain_image_views[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
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
