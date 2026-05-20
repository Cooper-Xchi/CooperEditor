#include "engine/vulkan/VulkanContextInternal.hpp"

#include "engine/config/AppConfig.hpp"

namespace engine::vulkan::detail {

VkRenderPass createRenderPass(VkDevice logicalDevice,
                              VkFormat colorAttachmentFormat) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorAttachmentFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    check(vkCreateRenderPass(logicalDevice,
                             &renderPassInfo,
                             nullptr,
                             &renderPass),
          "vkCreateRenderPass failed");
    return renderPass;
}

std::vector<VkFramebuffer> createFramebuffers(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainImageViews,
    VkExtent2D extent) {
    std::vector<VkFramebuffer> framebuffers;
    framebuffers.reserve(swapchainImageViews.size());

    try {
        for (const auto imageView : swapchainImageViews) {
            VkImageView attachments[] = {imageView};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            check(vkCreateFramebuffer(logicalDevice,
                                      &framebufferInfo,
                                      nullptr,
                                      &framebuffer),
                  "vkCreateFramebuffer failed");
            framebuffers.push_back(framebuffer);
        }
    } catch (...) {
        for (auto& framebuffer : framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
            }
        }
        throw;
    }

    return framebuffers;
}

VkCommandPool createCommandPool(VkDevice logicalDevice,
                                uint32_t graphicsQueueFamilyIndex) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    check(vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool),
          "vkCreateCommandPool failed");
    return commandPool;
}

std::vector<VkCommandBuffer> allocateCommandBuffers(
    VkDevice logicalDevice,
    VkCommandPool commandPool,
    uint32_t commandBufferCount) {
    std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    check(vkAllocateCommandBuffers(logicalDevice,
                                   &allocInfo,
                                   commandBuffers.data()),
          "vkAllocateCommandBuffers failed");
    return commandBuffers;
}

void recordCommandBuffers(VkRenderPass renderPass,
                          VkExtent2D extent,
                          const std::vector<VkFramebuffer>& framebuffers,
                          const std::vector<VkCommandBuffer>& commandBuffers,
                          VkPipelineLayout pipelineLayout,
                          VkPipeline graphicsPipeline,
                          VkBuffer vertexBuffer,
                          const std::vector<VkDescriptorSet>& cameraDescriptorSets,
                          uint32_t drawVertexCount) {
    const auto& clearColorConfig = engine::config::appConfig().render.clearColor;

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(commandBuffers.size());
         ++index) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        check(vkBeginCommandBuffer(commandBuffers[index], &beginInfo),
              "vkBeginCommandBuffer failed");

        VkClearValue clearColor{};
        clearColor.color = {{clearColorConfig[0],
                             clearColorConfig[1],
                             clearColorConfig[2],
                             clearColorConfig[3]}};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[index];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[index],
                             &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(commandBuffers[index],
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              graphicsPipeline);
            if (vertexBuffer != VK_NULL_HANDLE) {
                const VkBuffer vertexBuffers[] = {vertexBuffer};
                const VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffers[index],
                                       0,
                                       1,
                                       vertexBuffers,
                                       offsets);
            }
            if (!cameraDescriptorSets.empty()) {
                vkCmdBindDescriptorSets(commandBuffers[index],
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout,
                                        0,
                                        1,
                                        &cameraDescriptorSets[index],
                                        0,
                                        nullptr);
            }
            vkCmdDraw(commandBuffers[index], drawVertexCount, 1, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffers[index]);

        check(vkEndCommandBuffer(commandBuffers[index]),
              "vkEndCommandBuffer failed");
    }
}

VkSemaphore createSemaphore(VkDevice logicalDevice) {
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    check(vkCreateSemaphore(logicalDevice, &createInfo, nullptr, &semaphore),
          "vkCreateSemaphore failed");
    return semaphore;
}

VkFence createFence(VkDevice logicalDevice) {
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence fence = VK_NULL_HANDLE;
    check(vkCreateFence(logicalDevice, &createInfo, nullptr, &fence),
          "vkCreateFence failed");
    return fence;
}

std::vector<FrameSyncObjects> createFrameSyncObjects(VkDevice logicalDevice,
                                                     uint32_t frameCount) {
    std::vector<FrameSyncObjects> frames(frameCount);
    try {
        for (auto& frame : frames) {
            frame.imageAvailableSemaphore = createSemaphore(logicalDevice);
            frame.renderFinishedSemaphore = createSemaphore(logicalDevice);
            frame.inFlightFence = createFence(logicalDevice);
        }
    } catch (...) {
        for (auto& frame : frames) {
            if (frame.inFlightFence != VK_NULL_HANDLE) {
                vkDestroyFence(logicalDevice, frame.inFlightFence, nullptr);
                frame.inFlightFence = VK_NULL_HANDLE;
            }
            if (frame.renderFinishedSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(logicalDevice,
                                   frame.renderFinishedSemaphore,
                                   nullptr);
                frame.renderFinishedSemaphore = VK_NULL_HANDLE;
            }
            if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(logicalDevice,
                                   frame.imageAvailableSemaphore,
                                   nullptr);
                frame.imageAvailableSemaphore = VK_NULL_HANDLE;
            }
        }
        throw;
    }
    return frames;
}

    //绘制单帧
FrameSubmitStatus submitSingleFrame(VkDevice logicalDevice,
                                    VkQueue graphicsQueue,
                                    VkQueue presentQueue,
                                    VkSwapchainKHR swapchain,
                                    const std::vector<VkCommandBuffer>& commandBuffers,
                                    std::vector<FrameSyncObjects>& framesInFlight,
                                    std::vector<VkFence>& imagesInFlight,
                                    uint32_t& currentFrameIndex) {
    auto& currentFrame = framesInFlight[currentFrameIndex];

    check(vkWaitForFences(logicalDevice,
                          1,
                          &currentFrame.inFlightFence,
                          VK_TRUE,
                          UINT64_MAX),
          "vkWaitForFences failed");
    check(vkResetFences(logicalDevice, 1, &currentFrame.inFlightFence),
          "vkResetFences failed");

    uint32_t imageIndex = 0;
    bool recreateSwapchain = false;
    const auto acquireResult =
        vkAcquireNextImageKHR(logicalDevice,
                              swapchain,
                              UINT64_MAX,
                              currentFrame.imageAvailableSemaphore,
                              VK_NULL_HANDLE,
                              &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return FrameSubmitStatus::RecreateSwapchain;
    }
    if (acquireResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain = true;
    } else {
        check(acquireResult, "vkAcquireNextImageKHR failed");
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        check(vkWaitForFences(logicalDevice,
                              1,
                              &imagesInFlight[imageIndex],
                              VK_TRUE,
                              UINT64_MAX),
              "vkWaitForFences for swapchain image failed");
    }
    imagesInFlight[imageIndex] = currentFrame.inFlightFence;

    VkSemaphore waitSemaphores[] = {currentFrame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkSemaphore signalSemaphores[] = {currentFrame.renderFinishedSemaphore};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    check(vkQueueSubmit(graphicsQueue,
                        1,
                        &submitInfo,
                        currentFrame.inFlightFence),
          "vkQueueSubmit failed");

    VkSwapchainKHR swapchains[] = {swapchain};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    const auto presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain = true;
    } else {
        check(presentResult, "vkQueuePresentKHR failed");
    }

    currentFrameIndex =
        (currentFrameIndex + 1) % static_cast<uint32_t>(framesInFlight.size());
    if (recreateSwapchain) {
        return FrameSubmitStatus::RecreateSwapchain;
    }
    return FrameSubmitStatus::Submitted;
}

}  // namespace engine::vulkan::detail
