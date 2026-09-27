#include "vulkanapp.h"

void VulkanApp::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding uniform_buffer_binding = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo DSL_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    DSL_create_info.bindingCount = 1;
    DSL_create_info.pBindings = &uniform_buffer_binding;
    vkCreateDescriptorSetLayout(device, &DSL_create_info, nullptr, &descriptor_set_layout);
}

void VulkanApp::create_command_pool()
{
    VkCommandPoolCreateInfo command_pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = queue_family_info.graphics_index;
    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        printf("failed to create command pool");
    }
}

void VulkanApp::create_command_buffers()
{
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data());
}

u32 VulkanApp::find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties)
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

MemoryBuffer VulkanApp::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
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

void VulkanApp::copy_buffer(MemoryBuffer src_buffer, MemoryBuffer dest_buffer, VkDeviceSize buffer_size)
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

void VulkanApp::create_vertex_buffer()
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

void VulkanApp::create_index_buffer()
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

void VulkanApp::create_uniform_buffers()
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

void VulkanApp::create_descriptor_pool()
{
    VkDescriptorPoolSize DP_size = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (u32)MAX_FRAMES_IN_FLIGHT};
    VkDescriptorPoolCreateInfo DP_create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    DP_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DP_create_info.maxSets = MAX_FRAMES_IN_FLIGHT;
    DP_create_info.poolSizeCount = 1;
    DP_create_info.pPoolSizes = &DP_size;

    vkCreateDescriptorPool(device, &DP_create_info, nullptr, &descriptor_pool);
}

void VulkanApp::create_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT);
    for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        layouts[i] = descriptor_set_layout;
    }
    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = layouts.size();
    alloc_info.pSetLayouts = layouts.data();
    descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data());

    for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info = {uniform_buffers[i].buffer, 0, sizeof(UniformBuffer)};
        VkWriteDescriptorSet write_descriptor_set = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write_descriptor_set.dstSet = descriptor_sets[i];
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_set.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
    }
}

void VulkanApp::record_command_buffer(VkCommandBuffer command_buffer, u32 image_index)
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

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[frame_index], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, indices.size(), 1, 0, 0, 0);

    vkCmdEndRendering(command_buffer);

    transition_image_layout(command_buffer, image_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, {}, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(command_buffer);
}
