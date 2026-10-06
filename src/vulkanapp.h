#pragma once
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
#include "core.h"
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

struct Vertex
{
    f32 position[2];
    f32 color[3];
};

struct UniformBuffer
{
    glm::mat4 model;
    glm::mat4 camera;
    glm::mat4 proj;
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
    // INITIALISATION
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void pick_physical_device();
    void create_device();
    void create_swapchain();
    void create_image_views();

    // PIPELINE CREATION
    void create_graphics_pipeline();
    void transition_image_layout(VkCommandBuffer command_buffer, u32 image_index, VkImageLayout old_layout, VkImageLayout new_layout,
                                 VkAccessFlags2 src_access_mask, VkAccessFlags2 dest_access_mask, VkPipelineStageFlags2 src_stage_mask,
                                 VkPipelineStageFlags2 dest_stage_mask);

    // BUFFER MANAGEMENT
    void create_descriptor_set_layout();
    void create_command_pool();
    void create_command_buffers();
    u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);
    MemoryBuffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
    void copy_buffer(MemoryBuffer src_buffer, MemoryBuffer dest_buffer, VkDeviceSize buffer_size);
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index);

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
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
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
        vkFreeDescriptorSets(device, descriptor_pool, 1, descriptor_sets.data());
        vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

        for (i32 i = 0; i < swapchain_image_views.size(); i++)
        {
            vkDestroyImageView(device, swapchain_image_views[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyBuffer(device, vertex_buffer.buffer, nullptr);
        vkFreeMemory(device, vertex_buffer.memory, nullptr);
        vkDestroyBuffer(device, index_buffer.buffer, nullptr);
        vkFreeMemory(device, index_buffer.memory, nullptr);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(device, uniform_buffers[i].buffer, nullptr);
            vkFreeMemory(device, uniform_buffers[i].memory, nullptr);
        }
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

    void update_uniform_buffer(u32 frame_index, glm::mat4 camera)
    {
        UniformBuffer ubo;
        ubo.model = {{cos(delta_time), -sin(delta_time), 0, 0}, {sin(delta_time), cos(delta_time), 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
        ubo.camera = camera;
        ubo.proj = {{2.4142, 0, 0, 0}, {0, 2.4142, 0, 0}, {0, 0, 1.2222, 2.2222}, {0, 0, 1, 0}};

        memcpy(uniform_buffer_map[frame_index], &ubo, sizeof(UniformBuffer));
    }

    void render(glm::mat4 camera_position)
    {

        vkQueueWaitIdle(queue_family_info.graphics_queue);
        vkDeviceWaitIdle(device);

        delta_time += 0.0001f;
        u32 image_index;
        vkWaitForFences(device, 1, &fences_drawing_complete[frame_index], VK_TRUE, UINT64_MAX);

        VkResult acquire_next_image_result =
            vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores_presentation_complete[frame_index], nullptr, &image_index);

        if (acquire_next_image_result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreate_swapchain(); // TODO: There is no easy way to debug this currently on my system, need to test on other devices.
            return;
        }

        update_uniform_buffer(frame_index, camera_position);


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


        vkQueueSubmit(queue_family_info.graphics_queue, 1, &submit_info, fences_drawing_complete[frame_index]);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_rendering_finished[image_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;

        vkQueuePresentKHR(queue_family_info.presentation_queue, &present_info);

        frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;

        return;
    }

    void update_camera(glm::mat4 *camera_position, Input input)
    {
        glm::mat4 camera_location = {
            {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0.001 * input.a - 0.001 * input.d, 0, 0.001 * input.s - 0.001 * input.w, 1}};
        glm::mat4 camera_xrot = {{cos(0.0001 * input.left - 0.0001 * input.right), 0, -sin(0.0001 * input.left - 0.0001 * input.right), 0},
                                 {0, 1, 0, 0},
                                 {sin(0.0001 * input.left - 0.0001 * input.right), 0, cos(0.0001 * input.left - 0.0001 * input.right), 0},
                                 {0, 0, 0, 1}};
        glm::mat4 camera_yrot = {{1, 0, 0, 0},
                                 {0, cos(0.0001 * input.up - 0.0001 * input.down), -sin(0.0001 * input.up - 0.0001 * input.down), 0},
                                 {0, sin(0.0001 * input.up - 0.0001 * input.down), cos(0.0001 * input.up - 0.0001 * input.down), 0},
                                 {0, 0, 0, 1}};
        *camera_position = camera_location * (*camera_position);
        *camera_position = camera_xrot * (*camera_position);
        *camera_position = camera_yrot * (*camera_position);
    }
};
