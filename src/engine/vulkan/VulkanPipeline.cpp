#include "engine/vulkan/VulkanContextInternal.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace engine::vulkan::detail {

VkDescriptorSetLayout createCameraDescriptorSetLayout(VkDevice logicalDevice) {
    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 0;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &cameraBinding;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    check(vkCreateDescriptorSetLayout(logicalDevice,
                                      &createInfo,
                                      nullptr,
                                      &descriptorSetLayout),
          "vkCreateDescriptorSetLayout failed");
    return descriptorSetLayout;
}

VkDescriptorPool createCameraDescriptorPool(VkDevice logicalDevice,
                                            uint32_t descriptorCount) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
    createInfo.maxSets = descriptorCount;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    check(vkCreateDescriptorPool(logicalDevice,
                                 &createInfo,
                                 nullptr,
                                 &descriptorPool),
          "vkCreateDescriptorPool failed");
    return descriptorPool;
}

std::vector<VkDescriptorSet> allocateCameraDescriptorSets(
    VkDevice logicalDevice,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    const std::vector<BufferResource>& cameraUniformBuffers) {
    std::vector<VkDescriptorSetLayout> layouts(cameraUniformBuffers.size(),
                                               descriptorSetLayout);
    std::vector<VkDescriptorSet> descriptorSets(cameraUniformBuffers.size());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocateInfo.pSetLayouts = layouts.data();

    check(vkAllocateDescriptorSets(logicalDevice,
                                   &allocateInfo,
                                   descriptorSets.data()),
          "vkAllocateDescriptorSets failed");

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(cameraUniformBuffers.size());
         ++index) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = cameraUniformBuffers[index].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraUniformData);

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSets[index];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }

    return descriptorSets;
}

VkPipelineLayout createPipelineLayout(VkDevice logicalDevice,
                                      VkDescriptorSetLayout descriptorSetLayout) {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    check(vkCreatePipelineLayout(logicalDevice,
                                 &createInfo,
                                 nullptr,
                                 &pipelineLayout),
          "vkCreatePipelineLayout failed");
    return pipelineLayout;
}

namespace {

std::string shaderPath(const char* filename) {
    return std::string(ENGINE_SHADER_DIR) + "/" + filename;
}

}  // namespace

VkPipeline createGraphicsPipeline(VkDevice logicalDevice,
                                  VkExtent2D extent,
                                  VkRenderPass renderPass,
                                  VkPipelineLayout pipelineLayout,
                                  const std::string& vertexShaderFile,
                                  const std::string& fragmentShaderFile) {
    const auto vertexShaderCode = readBinaryFile(shaderPath(vertexShaderFile.c_str()));
    const auto fragmentShaderCode =
        readBinaryFile(shaderPath(fragmentShaderFile.c_str()));

    const auto vertexShaderModule =
        createShaderModule(logicalDevice, vertexShaderCode);
    const auto fragmentShaderModule =
        createShaderModule(logicalDevice, fragmentShaderCode);

    try {
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo,
        };

        VkVertexInputBindingDescription vertexBindingDescription{};
        vertexBindingDescription.binding = 0;
        vertexBindingDescription.stride = sizeof(engine::render::Vertex);
        vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        const std::array<VkVertexInputAttributeDescription, 2>
            vertexAttributeDescriptions = {{
                {
                    0,
                    0,
                    VK_FORMAT_R32G32_SFLOAT,
                    static_cast<uint32_t>(
                        offsetof(engine::render::Vertex, position)),
                },
                {
                    1,
                    0,
                    VK_FORMAT_R32G32B32_SFLOAT,
                    static_cast<uint32_t>(
                        offsetof(engine::render::Vertex, color)),
                },
            }};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(vertexAttributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions =
            vertexAttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        check(vkCreateGraphicsPipelines(logicalDevice,
                                        VK_NULL_HANDLE,
                                        1,
                                        &pipelineInfo,
                                        nullptr,
                                        &graphicsPipeline),
              "vkCreateGraphicsPipelines failed");

        vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
        return graphicsPipeline;
    } catch (...) {
        vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
        throw;
    }
}

}  // namespace engine::vulkan::detail
