
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

struct Vertex
{
    f32 position[2];
    f32 color[3];
};

struct vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

// NOTE: Spriv uses a column major order, so these vectors are the columns of the matrix
struct mat4
{
    vec4 x;
    vec4 y;
    vec4 z;
    vec4 w;
};

struct UniformBuffer
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct VertexInfo
{
    u32 attribute_count = 2;
    VkVertexInputAttributeDescription attributes[2];
    u32 binding_count = 1;
    VkVertexInputBindingDescription bindings[1];

    VertexInfo()
    {
        attributes[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)};
        attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};
        bindings[0].binding = 0;
        bindings[0].stride = sizeof(Vertex);
        bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
};

struct MemoryBuffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

const std::vector<u16> indices = {0, 1, 2, 2, 3, 0};

void *open_file(const char *file_name, i32 *file_size)
{
    FILE *fptr = fopen(file_name, "rb");
    fseek(fptr, 0L, SEEK_END);
    *file_size = ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);
    void *file_contents = malloc(*file_size);
    fread(file_contents, 1, *file_size, fptr);
    fclose(fptr);
    return file_contents;
}

void close_file(void *file)
{
    free(file);
    return;
}

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
        u32 transfer_index;
        VkQueue transfer_queue;
    };

    struct SwapchainInfo
    {
        VkSurfaceFormatKHR surface_format;
        VkExtent2D extent;
    };

  private:

    f32 delta_time;
    const i32 MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<char const *> instance_extensions = {"VK_EXT_debug_utils"};
    std::vector<char const *> instance_layers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> device_extensions = {"VK_KHR_swapchain", "VK_EXT_extended_dynamic_state"};

    VertexInfo vertex_info;

    Window *window;

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
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkCommandPool command_pool;

    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> semaphores_presentation_complete;
    std::vector<VkSemaphore> semaphores_rendering_finished;
    std::vector<VkFence> fences_drawing_complete;

    u32 frame_index = 0;

    MemoryBuffer vertex_buffer;
    MemoryBuffer index_buffer;
    std::vector<MemoryBuffer> uniform_buffers;
    std::vector<void *> uniform_buffer_map;

    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

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

    void create_surface()
    {
        VkWaylandSurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        surface_create_info.display = glfwGetWaylandDisplay();
        surface_create_info.surface = glfwGetWaylandWindow(window->get_glfw_window());

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

        queue_family_info.transfer_index = queue_family_info.graphics_index;

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
        vulkan_13_features.synchronization2 = true;
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

        queue_info.queueFamilyIndex = queue_family_info.transfer_index;

        vkGetDeviceQueue2(device, &queue_info, &queue_family_info.transfer_queue);

        return;
    }

    void create_swapchain()
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
        glfwGetFramebufferSize(window->get_glfw_window(), &width, &height);
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
        VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = swapchain_info.surface_format.format;
        image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        image_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                             VK_COMPONENT_SWIZZLE_IDENTITY};
        image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        for (i32 i = 0; i < swapchain_images.size(); i++)
        {
            image_view_create_info.image = swapchain_images[i];
            vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]);
        }
    }

    void create_descriptor_set_layout()
    {
        VkDescriptorSetLayoutBinding uniform_buffer_binding = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo DSL_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        DSL_create_info.bindingCount = 1;
        DSL_create_info.pBindings = &uniform_buffer_binding;
        vkCreateDescriptorSetLayout(device, &DSL_create_info, nullptr, &descriptor_set_layout);
    }

    void create_graphics_pipeline()
    {
        i32 shader_code_size;
        void *shader_code = open_file("bin/slang.spv", &shader_code_size);

        VkShaderModuleCreateInfo shader_module_create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader_module_create_info.codeSize = shader_code_size;
        shader_module_create_info.pCode = (u32 *)shader_code;

        VkShaderModule shader_module;
        vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module);

        VkPipelineShaderStageCreateInfo shader_stages[2];

        shader_stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader_stages[0].module = shader_module;
        shader_stages[0].pName = "vert_main";

        shader_stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader_stages[1].module = shader_module;
        shader_stages[1].pName = "frag_main";

        std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic_state_create_info.dynamicStateCount = dynamic_states.size();
        dynamic_state_create_info.pDynamicStates = dynamic_states.data();

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_info.binding_count;
        vertex_input_state_create_info.pVertexBindingDescriptions = vertex_info.bindings;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_info.attribute_count;
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_info.attributes;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state_create_info.sampleShadingEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
        color_blend_attachment_state.blendEnable = VK_FALSE;
        color_blend_attachment_state.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        color_blend_state_create_info.logicOpEnable = VK_FALSE;
        color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        color_blend_state_create_info.attachmentCount = 1;
        color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout);

        VkPipelineRenderingCreateInfo rendering_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering_create_info.colorAttachmentCount = 1;
        rendering_create_info.pColorAttachmentFormats = &swapchain_info.surface_format.format;

        VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = shader_stages;
        graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        graphics_pipeline_create_info.layout = pipeline_layout;
        graphics_pipeline_create_info.renderPass = nullptr;
        graphics_pipeline_create_info.pNext = &rendering_create_info;

        if (vkCreateGraphicsPipelines(device, nullptr, 1, &graphics_pipeline_create_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
        {
            printf("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(device, shader_module, nullptr);
        close_file(shader_code);
        return;
    }

    void create_command_pool()
    {
        VkCommandPoolCreateInfo command_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_create_info.queueFamilyIndex = queue_family_info.graphics_index;
        if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
        {
            printf("failed to create command pool");
        }
    }

    void create_command_buffers()
    {
        command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo command_buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data());
    }

    void transition_image_layout(VkCommandBuffer command_buffer, u32 image_index, VkImageLayout old_layout, VkImageLayout new_layout,
                                 VkAccessFlags2 src_access_mask, VkAccessFlags2 dest_access_mask, VkPipelineStageFlags2 src_stage_mask,
                                 VkPipelineStageFlags2 dest_stage_mask)
    {
        VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                         nullptr,
                                         src_stage_mask,
                                         src_access_mask,
                                         dest_stage_mask,
                                         dest_access_mask,
                                         old_layout,
                                         new_layout,
                                         VK_QUEUE_FAMILY_IGNORED,
                                         VK_QUEUE_FAMILY_IGNORED,
                                         swapchain_images[image_index],
                                         {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
        VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.dependencyFlags = {};
        dependency_info.imageMemoryBarrierCount = 1;
        dependency_info.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(command_buffer, &dependency_info);
    }

    u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        for (i32 i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) && ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties))
            {
                return i;
            }
        }
        printf("No valid memory type found");
        return -1;
    }

    MemoryBuffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
    {
        MemoryBuffer memory_buffer;

        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size = size;
        create_info.usage = usage_flags;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(device, &create_info, nullptr, &memory_buffer.buffer);

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, memory_buffer.buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, memory_flags);

        vkAllocateMemory(device, &allocate_info, nullptr, &memory_buffer.memory);
        vkBindBufferMemory(device, memory_buffer.buffer, memory_buffer.memory, 0);

        return memory_buffer;
    }

    void copy_buffer(MemoryBuffer src_buffer, MemoryBuffer dest_buffer, VkDeviceSize buffer_size)
    {
        VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocate_info.commandPool = command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer copy_buffer;
        vkAllocateCommandBuffers(device, &allocate_info, &copy_buffer);

        VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkBufferCopy copy_spec = {0, 0, buffer_size};

        vkBeginCommandBuffer(copy_buffer, &begin_info);
        vkCmdCopyBuffer(copy_buffer, src_buffer.buffer, dest_buffer.buffer, 1, &copy_spec);
        vkEndCommandBuffer(copy_buffer);

        VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &copy_buffer;

        vkQueueSubmit(queue_family_info.transfer_queue, 1, &submit_info, nullptr);
    }

    void create_vertex_buffer()
    {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        MemoryBuffer staging_buffer =
            create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        vertex_buffer =
            create_buffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        void *data;
        vkMapMemory(device, staging_buffer.memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices.data(), buffer_size);
        vkUnmapMemory(device, staging_buffer.memory);

        copy_buffer(staging_buffer, vertex_buffer, buffer_size);
        vkDeviceWaitIdle(device);
        vkDestroyBuffer(device, staging_buffer.buffer, nullptr);
        vkFreeMemory(device, staging_buffer.memory, nullptr);
    }

    void create_index_buffer()
    {
        VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        MemoryBuffer staging_buffer =
            create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        index_buffer =
            create_buffer(buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        void *data;
        vkMapMemory(device, staging_buffer.memory, 0, buffer_size, 0, &data);
        memcpy(data, indices.data(), buffer_size);
        vkUnmapMemory(device, staging_buffer.memory);

        copy_buffer(staging_buffer, index_buffer, buffer_size);
        vkDeviceWaitIdle(device);
        vkDestroyBuffer(device, staging_buffer.buffer, nullptr);
        vkFreeMemory(device, staging_buffer.memory, nullptr);
    }

    void create_uniform_buffers()
    {
        for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDeviceSize buffer_size = sizeof(UniformBuffer);
            MemoryBuffer buffer = create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uniform_buffers.emplace_back(std::move(buffer));
            void *buffer_map;
            vkMapMemory(device, uniform_buffers.back().memory, 0, buffer_size, 0, &buffer_map);
            uniform_buffer_map.emplace_back(buffer_map);
        }
    }

    void create_descriptor_pool()
    {
        VkDescriptorPoolSize DP_size = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (u32)MAX_FRAMES_IN_FLIGHT};
        VkDescriptorPoolCreateInfo DP_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        DP_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        DP_create_info.maxSets = MAX_FRAMES_IN_FLIGHT;
        DP_create_info.poolSizeCount = 1;
        DP_create_info.pPoolSizes = &DP_size;

        vkCreateDescriptorPool(device, &DP_create_info, nullptr, &descriptor_pool);
    }

    void create_descriptor_sets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT);
        for (i32 i =0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            layouts[i] = descriptor_set_layout;
        }
        VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc_info.descriptorPool = descriptor_pool;
        alloc_info.descriptorSetCount = layouts.size();
        alloc_info.pSetLayouts = layouts.data();
        descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
        vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data());

        for (i32 i =0; i< MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo buffer_info = {uniform_buffers[i].buffer, 0, sizeof(UniformBuffer)};
        }
    }

    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        transition_image_layout(command_buffer, image_index, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, {},
                                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
        VkRenderingAttachmentInfo attachment_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        attachment_info.imageView = swapchain_image_views[image_index];
        attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_info.clearValue = clear_color;

        VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering_info.renderArea = {{0, 0}, swapchain_info.extent};
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &attachment_info;

        vkCmdBeginRendering(command_buffer, &rendering_info);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        VkDeviceSize offsets[1] = {0};

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(command_buffer, index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport = {0.0f, 0.0f, (f32)swapchain_info.extent.width, (f32)swapchain_info.extent.height, 0.0f, 1.0f};

        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {{0, 0}, swapchain_info.extent};

        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        vkCmdDrawIndexed(command_buffer, indices.size(), 1, 0, 0, 0);

        vkCmdEndRendering(command_buffer);

        transition_image_layout(command_buffer, image_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, {}, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

        vkEndCommandBuffer(command_buffer);
    }

    void create_sync_objects()
    {
        semaphores_presentation_complete.resize(MAX_FRAMES_IN_FLIGHT);
        semaphores_rendering_finished.resize(swapchain_images.size());
        fences_drawing_complete.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphores_presentation_complete[i]);
            vkCreateFence(device, &fence_create_info, nullptr, &fences_drawing_complete[i]);
        }

        for (i32 i = 0; i < swapchain_images.size(); i++)
        {

            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphores_rendering_finished[i]);
        }
    }

    void recreate_swapchain()
    {
        vkDeviceWaitIdle(device);
        for (i32 i = 0; i < swapchain_image_views.size(); i++)
        {
            vkDestroyImageView(device, swapchain_image_views[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        create_swapchain();
        create_image_views();
    }

  public:
    VulkanApp(Window *external_window)
    {
        window = external_window;
        create_instance();
        create_debug_messenger();
        create_surface();
        pick_physical_device();
        create_device();
        create_swapchain();
        create_image_views();
        create_descriptor_set_layout();
        create_graphics_pipeline();
        create_command_pool();
        create_vertex_buffer();
        create_index_buffer();
        create_command_buffers();
        create_sync_objects();
    }

    ~VulkanApp()
    {
        vkQueueWaitIdle(queue_family_info.graphics_queue);
        for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, semaphores_presentation_complete[i], nullptr);
            vkDestroyFence(device, fences_drawing_complete[i], nullptr);
        }

        for (i32 i = 0; i < swapchain_images.size(); i++)
        {

            vkDestroySemaphore(device, semaphores_rendering_finished[i], nullptr);
        }

        vkDestroyCommandPool(device, command_pool, nullptr);
        vkDestroyPipeline(device, graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

        for (i32 i = 0; i < swapchain_image_views.size(); i++)
        {
            vkDestroyImageView(device, swapchain_image_views[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyBuffer(device, vertex_buffer.buffer, nullptr);
        vkFreeMemory(device, vertex_buffer.memory, nullptr);
        vkDestroyBuffer(device, index_buffer.buffer, nullptr);
        vkFreeMemory(device, index_buffer.memory, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);

        PFN_vkDestroyDebugUtilsMessengerEXT messenger_destroy_function =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (messenger_destroy_function == 0)
        {
            printf("Unable to locate debug messenger destroy function\n");
        }

        messenger_destroy_function(instance, debug_messenger, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    void update_uniform_buffer(u32 frame_index)
    {
        UniformBuffer ubo;
        ubo.model.x = {cos(delta_time), -sin(delta_time), 0, 0};
        ubo.model.y = {sin(delta_time), cos(delta_time), 0, 0};
        ubo.model.z = {0, 0, 1, 0};
        ubo.model.w = {0, 0, 0, 1};

        ubo.view = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
        ubo.proj = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};

        memcpy(uniform_buffer_map[frame_index], &ubo, sizeof(UniformBuffer));
    }

    void render()
    {
        delta_time += 0.0001f;
        u32 image_index;
        vkWaitForFences(device, 1, &fences_drawing_complete[frame_index], VK_TRUE, UINT64_MAX);

        VkResult acquire_next_image_result =
            vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores_presentation_complete[frame_index], nullptr, &image_index);

        vkResetFences(device, 1, &fences_drawing_complete[frame_index]);

        vkResetCommandBuffer(command_buffers[frame_index], 0);
        record_command_buffer(command_buffers[frame_index], image_index);

        VkPipelineStageFlags wait_destination_stage_mask = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_presentation_complete[frame_index];
        submit_info.pWaitDstStageMask = &wait_destination_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers[frame_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_rendering_finished[image_index];

        update_uniform_buffer(frame_index);

        vkQueueSubmit(queue_family_info.graphics_queue, 1, &submit_info, fences_drawing_complete[frame_index]);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_rendering_finished[image_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;

        vkQueuePresentKHR(queue_family_info.presentation_queue, &present_info);

        frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;

        if (acquire_next_image_result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreate_swapchain(); // TODO: There is no easy way to debug this currently on my system, need to test on other devices.
        }

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
    }

    delete vulkan_app;
    delete glfw_window;
    return 0;
}
