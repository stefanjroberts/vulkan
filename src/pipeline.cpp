#include "vulkanapp.h"

void VulkanApp::create_graphics_pipeline()
{
    i32 shader_code_size;
    void *shader_code = open_file("./slang.spv", &shader_code_size);

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

void VulkanApp::transition_image_layout(VkCommandBuffer command_buffer, u32 image_index, VkImageLayout old_layout, VkImageLayout new_layout,
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
