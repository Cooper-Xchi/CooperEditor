#include "engine/vulkan/VulkanContext.hpp"

#include <cstring>

#include "engine/vulkan/VulkanContextInternal.hpp"
#include "engine/window/Window.hpp"

namespace engine::vulkan {

bool VulkanContext::waitForValidFramebufferSize() const {
    if (window_ == nullptr) {
        return false;
    }

    auto [width, height] = window_->framebufferSize();
    while (width == 0 || height == 0) {
        if (window_->shouldClose()) {
            return false;
        }
        window_->waitEvents();
        std::tie(width, height) = window_->framebufferSize();
    }

    return true;
}

DeviceRuntime* VulkanContext::primaryDeviceRuntime() noexcept {
    if (!primaryDeviceRuntime_.has_value()) {
        return nullptr;
    }
    return &primaryDeviceRuntime_.value();
}

const DeviceRuntime* VulkanContext::primaryDeviceRuntime() const noexcept {
    if (!primaryDeviceRuntime_.has_value()) {
        return nullptr;
    }
    return &primaryDeviceRuntime_.value();
}

void VulkanContext::uploadUniforms(
    const std::vector<BufferResource>& uniformBuffers,
    const CameraUniformData& uniform) {
    for (const auto& uniformBuffer : uniformBuffers) {
        if (uniformBuffer.mappedData != nullptr) {
            std::memcpy(uniformBuffer.mappedData,
                        &uniform,
                        sizeof(CameraUniformData));
        }
    }
}

void VulkanContext::destroyFrameSyncObjects(DeviceRuntime& runtime) {
    for (auto& frame : runtime.framesInFlight) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(runtime.logicalDevice, frame.inFlightFence, nullptr);
            frame.inFlightFence = VK_NULL_HANDLE;
        }
        if (frame.renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(runtime.logicalDevice,
                               frame.renderFinishedSemaphore,
                               nullptr);
            frame.renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(runtime.logicalDevice,
                               frame.imageAvailableSemaphore,
                               nullptr);
            frame.imageAvailableSemaphore = VK_NULL_HANDLE;
        }
    }
    runtime.framesInFlight.clear();
    runtime.imagesInFlight.clear();
    runtime.currentFrameIndex = 0;
    runtime.frameSubmitted = false;
}

void VulkanContext::destroySwapchainResources(DeviceRuntime& runtime) {
    destroyFrameSyncObjects(runtime);
    runtime.swapchainSelection.reset();
    runtime.swapchainMinImageCount = 0;

    if (runtime.descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(runtime.logicalDevice, runtime.descriptorPool, nullptr);
        runtime.descriptorPool = VK_NULL_HANDLE;
    }
    runtime.sceneCameraDescriptorSets.clear();
    runtime.gameCameraDescriptorSets.clear();

    for (auto& uniformBuffer : runtime.sceneCameraUniformBuffers) {
        if (uniformBuffer.mappedData != nullptr) {
            vkUnmapMemory(runtime.logicalDevice, uniformBuffer.memory);
            uniformBuffer.mappedData = nullptr;
        }
        if (uniformBuffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(runtime.logicalDevice, uniformBuffer.buffer, nullptr);
            uniformBuffer.buffer = VK_NULL_HANDLE;
        }
        if (uniformBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(runtime.logicalDevice, uniformBuffer.memory, nullptr);
            uniformBuffer.memory = VK_NULL_HANDLE;
        }
    }
    runtime.sceneCameraUniformBuffers.clear();

    for (auto& uniformBuffer : runtime.gameCameraUniformBuffers) {
        if (uniformBuffer.mappedData != nullptr) {
            vkUnmapMemory(runtime.logicalDevice, uniformBuffer.memory);
            uniformBuffer.mappedData = nullptr;
        }
        if (uniformBuffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(runtime.logicalDevice, uniformBuffer.buffer, nullptr);
            uniformBuffer.buffer = VK_NULL_HANDLE;
        }
        if (uniformBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(runtime.logicalDevice, uniformBuffer.memory, nullptr);
            uniformBuffer.memory = VK_NULL_HANDLE;
        }
    }
    runtime.gameCameraUniformBuffers.clear();

    if (runtime.commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(runtime.logicalDevice, runtime.commandPool, nullptr);
        runtime.commandPool = VK_NULL_HANDLE;
    }
    runtime.commandBuffers.clear();
    runtime.commandBuffersRecorded = false;

    if (runtime.swapchainGraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(runtime.logicalDevice,
                          runtime.swapchainGraphicsPipeline,
                          nullptr);
        runtime.swapchainGraphicsPipeline = VK_NULL_HANDLE;
    }
    if (runtime.sceneGraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(runtime.logicalDevice,
                          runtime.sceneGraphicsPipeline,
                          nullptr);
        runtime.sceneGraphicsPipeline = VK_NULL_HANDLE;
    }

    if (runtime.sceneViewportFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(runtime.logicalDevice,
                             runtime.sceneViewportFramebuffer,
                             nullptr);
        runtime.sceneViewportFramebuffer = VK_NULL_HANDLE;
    }
    if (runtime.gameViewportFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(runtime.logicalDevice,
                             runtime.gameViewportFramebuffer,
                             nullptr);
        runtime.gameViewportFramebuffer = VK_NULL_HANDLE;
    }

    for (auto& framebuffer : runtime.framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(runtime.logicalDevice, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }
    runtime.framebuffers.clear();

    if (runtime.sceneRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(runtime.logicalDevice,
                            runtime.sceneRenderPass,
                            nullptr);
        runtime.sceneRenderPass = VK_NULL_HANDLE;
    }
    if (runtime.swapchainRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(runtime.logicalDevice,
                            runtime.swapchainRenderPass,
                            nullptr);
        runtime.swapchainRenderPass = VK_NULL_HANDLE;
    }

    detail::destroyImageResource(runtime.logicalDevice,
                                 runtime.sceneViewportColorImage);
    detail::destroyImageResource(runtime.logicalDevice,
                                 runtime.gameViewportColorImage);

    for (auto& imageView : runtime.swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(runtime.logicalDevice, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
    }
    runtime.swapchainImageViews.clear();
    runtime.swapchainImages.clear();

    if (runtime.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(runtime.logicalDevice, runtime.swapchain, nullptr);
        runtime.swapchain = VK_NULL_HANDLE;
    }
}

void VulkanContext::createSwapchainResources(const DeviceInfo& device,
                                             DeviceRuntime& runtime) {
    if (device.swapchainSupport.formats.empty() ||
        device.swapchainSupport.presentModes.empty() ||
        !device.presentQueueFamilyIndex.has_value() ||
        !device.graphicsQueueFamilyIndex.has_value()) {
        return;
    }

    VkExtent2D framebufferExtent{};
    if (window_ != nullptr) {
        const auto [width, height] = window_->framebufferSize();
        framebufferExtent = {static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height)};
    }

    const auto selection =
        detail::chooseSwapchainSelection(device.swapchainSupport,
                                         framebufferExtent);
    runtime.swapchainSelection = selection;
    runtime.swapchainMinImageCount = selection.imageCount;
    runtime.swapchain =
        detail::createSwapchain(runtime.logicalDevice,
                                surface_,
                                device.graphicsQueueFamilyIndex.value(),
                                device.presentQueueFamilyIndex.value(),
                                runtime.swapchainSelection.value());
    runtime.swapchainImages =
        detail::querySwapchainImages(runtime.logicalDevice, runtime.swapchain);
    runtime.swapchainImageViews =
        detail::createSwapchainImageViews(
            runtime.logicalDevice,
            runtime.swapchainImages,
            selection.surfaceFormat.format);
    runtime.sceneRenderPass =
        detail::createRenderPass(runtime.logicalDevice,
                                 selection.surfaceFormat.format,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    runtime.swapchainRenderPass =
        detail::createRenderPass(runtime.logicalDevice,
                                 selection.surfaceFormat.format,
                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    runtime.sceneViewportColorImage =
        detail::createImageResource(device.physicalDevice,
                                    runtime.logicalDevice,
                                    selection.extent.width,
                                    selection.extent.height,
                                    selection.surfaceFormat.format,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
    runtime.gameViewportColorImage =
        detail::createImageResource(device.physicalDevice,
                                    runtime.logicalDevice,
                                    selection.extent.width,
                                    selection.extent.height,
                                    selection.surfaceFormat.format,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
    runtime.sceneGraphicsPipeline =
        detail::createGraphicsPipeline(runtime.logicalDevice,
                                       selection.extent,
                                       runtime.sceneRenderPass,
                                       runtime.pipelineLayout,
                                       runtime.vertexShaderFile,
                                       runtime.fragmentShaderFile);
    runtime.swapchainGraphicsPipeline =
        detail::createGraphicsPipeline(runtime.logicalDevice,
                                       selection.extent,
                                       runtime.swapchainRenderPass,
                                       runtime.pipelineLayout,
                                       runtime.vertexShaderFile,
                                       runtime.fragmentShaderFile);
    runtime.sceneCameraUniformBuffers.clear();
    runtime.gameCameraUniformBuffers.clear();
    runtime.sceneCameraUniformBuffers.reserve(runtime.swapchainImages.size());
    runtime.gameCameraUniformBuffers.reserve(runtime.swapchainImages.size());
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(runtime.swapchainImages.size());
         ++index) {
        runtime.sceneCameraUniformBuffers.push_back(
            detail::createBufferResource(device.physicalDevice,
                                         runtime.logicalDevice,
                                         sizeof(CameraUniformData),
                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
        runtime.gameCameraUniformBuffers.push_back(
            detail::createBufferResource(device.physicalDevice,
                                         runtime.logicalDevice,
                                         sizeof(CameraUniformData),
                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    }
    runtime.descriptorPool =
        detail::createCameraDescriptorPool(runtime.logicalDevice,
                                           static_cast<uint32_t>(
                                               runtime.swapchainImages.size() * 2));
    runtime.sceneCameraDescriptorSets =
        detail::allocateCameraDescriptorSets(runtime.logicalDevice,
                                             runtime.descriptorPool,
                                             runtime.descriptorSetLayout,
                                             runtime.sceneCameraUniformBuffers);
    runtime.gameCameraDescriptorSets =
        detail::allocateCameraDescriptorSets(runtime.logicalDevice,
                                             runtime.descriptorPool,
                                             runtime.descriptorSetLayout,
                                             runtime.gameCameraUniformBuffers);
    uploadUniforms(runtime.sceneCameraUniformBuffers, runtime.sceneCameraUniform);
    uploadUniforms(runtime.gameCameraUniformBuffers, runtime.gameCameraUniform);
    runtime.sceneViewportFramebuffer =
        detail::createFramebuffer(runtime.logicalDevice,
                                  runtime.sceneRenderPass,
                                  runtime.sceneViewportColorImage.imageView,
                                  selection.extent);
    runtime.gameViewportFramebuffer =
        detail::createFramebuffer(runtime.logicalDevice,
                                  runtime.sceneRenderPass,
                                  runtime.gameViewportColorImage.imageView,
                                  selection.extent);
    runtime.framebuffers =
        detail::createFramebuffers(runtime.logicalDevice,
                                   runtime.swapchainRenderPass,
                                   runtime.swapchainImageViews,
                                   selection.extent);
    runtime.commandPool =
        detail::createCommandPool(runtime.logicalDevice,
                                  device.graphicsQueueFamilyIndex.value());
    runtime.commandBuffers =
        detail::allocateCommandBuffers(
            runtime.logicalDevice,
            runtime.commandPool,
            static_cast<uint32_t>(runtime.framebuffers.size()));
    runtime.commandBuffersRecorded = true;
    runtime.framesInFlight =
        detail::createFrameSyncObjects(runtime.logicalDevice, 2);
    runtime.imagesInFlight.assign(runtime.swapchainImages.size(), VK_NULL_HANDLE);
}

void VulkanContext::recreateSwapchain(const DeviceInfo& device,
                                      DeviceRuntime& runtime) {
    if (!waitForValidFramebufferSize()) {
        return;
    }
    vkDeviceWaitIdle(runtime.logicalDevice);
    destroySwapchainResources(runtime);
    createSwapchainResources(device, runtime);
}

VulkanContext::~VulkanContext() {
    shutdown();
}

void VulkanContext::initialize(const engine::window::Window& window,
                               const engine::render::Scene& scene) {
    shutdown(); //环境清理 window surface instance 全部等待状态。设备为空

    try {
        window_ = &window;  //窗口实例
        scene_ = &scene;
        runtimeApiVersion_ = detail::queryRuntimeApiVersion(); //获取当前gpu驱动可获得的最高版本vulkanAPI
        createInstance();//创建实例instance
        createSurface(window);//创建实例surface
        discoverDevices();//发现设备
    } catch (...) {
        shutdown();
        throw;
    }
}

void VulkanContext::shutdown() {
    if (primaryDeviceRuntime_.has_value()) {
        auto& runtime = primaryDeviceRuntime_.value();
        if (runtime.logicalDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(runtime.logicalDevice);
        }
        destroySwapchainResources(runtime);
        if (runtime.pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(runtime.logicalDevice,
                                    runtime.pipelineLayout,
                                    nullptr);
            runtime.pipelineLayout = VK_NULL_HANDLE;
        }
        if (runtime.descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(runtime.logicalDevice,
                                         runtime.descriptorSetLayout,
                                         nullptr);
            runtime.descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (runtime.vertexBuffer.mappedData != nullptr) {
            vkUnmapMemory(runtime.logicalDevice, runtime.vertexBuffer.memory);
            runtime.vertexBuffer.mappedData = nullptr;
        }
        if (runtime.vertexBuffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(runtime.logicalDevice, runtime.vertexBuffer.buffer, nullptr);
            runtime.vertexBuffer.buffer = VK_NULL_HANDLE;
        }
        if (runtime.vertexBuffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(runtime.logicalDevice, runtime.vertexBuffer.memory, nullptr);
            runtime.vertexBuffer.memory = VK_NULL_HANDLE;
        }
        if (runtime.logicalDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(runtime.logicalDevice, nullptr);
            runtime.logicalDevice = VK_NULL_HANDLE;
            runtime.graphicsQueue = VK_NULL_HANDLE;
            runtime.presentQueue = VK_NULL_HANDLE;
            runtime.swapchainSelection.reset();
        }
    }
    primaryDeviceRuntime_.reset();
    devices_.clear();
    primaryDeviceIndex_.reset();
    window_ = nullptr;
    scene_ = nullptr;
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool VulkanContext::drawFrame(
    const std::function<void(VkCommandBuffer)>& overlayRecorder) {
    if (!primaryDeviceIndex_.has_value()) {
        return false;
    }

    auto* runtime = primaryDeviceRuntime();
    if (runtime == nullptr ||
        runtime->swapchain == VK_NULL_HANDLE ||
        runtime->commandBuffers.empty() ||
        runtime->framesInFlight.empty()) {
        return false;
    }

    const auto& device = devices_[primaryDeviceIndex_.value()];
    auto& currentFrame = runtime->framesInFlight[runtime->currentFrameIndex];

    detail::check(vkWaitForFences(runtime->logicalDevice,
                                  1,
                                  &currentFrame.inFlightFence,
                                  VK_TRUE,
                                  UINT64_MAX),
                  "vkWaitForFences failed");
    detail::check(vkResetFences(runtime->logicalDevice,
                                1,
                                &currentFrame.inFlightFence),
                  "vkResetFences failed");

    uint32_t imageIndex = 0;
    bool recreateSwapchainAfterPresent = false;
    const auto acquireResult =
        vkAcquireNextImageKHR(runtime->logicalDevice,
                              runtime->swapchain,
                              UINT64_MAX,
                              currentFrame.imageAvailableSemaphore,
                              VK_NULL_HANDLE,
                              &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(device, *runtime);
        return false;
    }
    if (acquireResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapchainAfterPresent = true;
    } else {
        detail::check(acquireResult, "vkAcquireNextImageKHR failed");
    }

    if (runtime->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        detail::check(vkWaitForFences(runtime->logicalDevice,
                                      1,
                                      &runtime->imagesInFlight[imageIndex],
                                      VK_TRUE,
                                      UINT64_MAX),
                      "vkWaitForFences for swapchain image failed");
    }
    runtime->imagesInFlight[imageIndex] = currentFrame.inFlightFence;

    detail::check(vkResetCommandBuffer(runtime->commandBuffers[imageIndex], 0),
                  "vkResetCommandBuffer failed");
    const VkDescriptorSet sceneCameraDescriptorSet =
        imageIndex < runtime->sceneCameraDescriptorSets.size()
            ? runtime->sceneCameraDescriptorSets[imageIndex]
            : VK_NULL_HANDLE;
    const VkDescriptorSet gameCameraDescriptorSet =
        imageIndex < runtime->gameCameraDescriptorSets.size()
            ? runtime->gameCameraDescriptorSets[imageIndex]
            : VK_NULL_HANDLE;
    detail::recordCommandBuffer(runtime->swapchainRenderPass,
                                runtime->swapchainSelection->extent,
                                runtime->framebuffers[imageIndex],
                                runtime->commandBuffers[imageIndex],
                                runtime->sceneRenderPass,
                                runtime->sceneViewportFramebuffer,
                                runtime->gameViewportFramebuffer,
                                runtime->sceneGraphicsPipeline,
                                runtime->swapchainSelection->extent,
                                runtime->pipelineLayout,
                                runtime->swapchainGraphicsPipeline,
                                runtime->vertexBuffer.buffer,
                                sceneCameraDescriptorSet,
                                gameCameraDescriptorSet,
                                runtime->sceneViewportColorImage.image,
                                runtime->gameViewportColorImage.image,
                                runtime->drawVertexCount,
                                overlayRecorder);

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
    submitInfo.pCommandBuffers = &runtime->commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    detail::check(vkQueueSubmit(runtime->graphicsQueue,
                                1,
                                &submitInfo,
                                currentFrame.inFlightFence),
                  "vkQueueSubmit failed");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &runtime->swapchain;
    presentInfo.pImageIndices = &imageIndex;

    const auto presentResult =
        vkQueuePresentKHR(runtime->presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        recreateSwapchainAfterPresent) {
        recreateSwapchain(device, *runtime);
    } else {
        detail::check(presentResult, "vkQueuePresentKHR failed");
    }

    runtime->currentFrameIndex =
        (runtime->currentFrameIndex + 1) %
        static_cast<uint32_t>(runtime->framesInFlight.size());
    runtime->frameSubmitted = true;
    return true;
}

void VulkanContext::handleFramebufferResize() {
    if (!primaryDeviceIndex_.has_value()) {
        return;
    }

    auto* runtime = primaryDeviceRuntime();
    if (runtime == nullptr ||
        runtime->logicalDevice == VK_NULL_HANDLE) {
        return;
    }

    const auto& device = devices_[primaryDeviceIndex_.value()];
    if (device.graphicsQueueFamilyIndex == std::nullopt ||
        device.presentQueueFamilyIndex == std::nullopt) {
        return;
    }

    recreateSwapchain(device, *runtime);
}

void VulkanContext::updateSceneViewUniform(const engine::math::Mat4& viewProjection,
                                           const engine::math::Mat4& model) {
    auto* runtime = primaryDeviceRuntime();
    if (runtime == nullptr) {
        return;
    }

    runtime->sceneCameraUniform.viewProjection = viewProjection;
    runtime->sceneCameraUniform.model = model;
    uploadUniforms(runtime->sceneCameraUniformBuffers, runtime->sceneCameraUniform);
}

void VulkanContext::updateGameViewUniform(const engine::math::Mat4& viewProjection,
                                          const engine::math::Mat4& model) {
    auto* runtime = primaryDeviceRuntime();
    if (runtime == nullptr) {
        return;
    }

    runtime->gameCameraUniform.viewProjection = viewProjection;
    runtime->gameCameraUniform.model = model;
    uploadUniforms(runtime->gameCameraUniformBuffers, runtime->gameCameraUniform);
}

uint32_t VulkanContext::runtimeApiVersion() const noexcept {
    return runtimeApiVersion_;
}

const std::vector<DeviceInfo>& VulkanContext::devices() const noexcept {
    return devices_;
}

VkInstance VulkanContext::instanceHandle() const noexcept {
    return instance_;
}

VkPhysicalDevice VulkanContext::primaryPhysicalDevice() const noexcept {
    if (!primaryDeviceIndex_.has_value()) {
        return VK_NULL_HANDLE;
    }
    return devices_[primaryDeviceIndex_.value()].physicalDevice;
}

VkDevice VulkanContext::primaryLogicalDevice() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->logicalDevice : VK_NULL_HANDLE;
}

VkQueue VulkanContext::primaryGraphicsQueue() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->graphicsQueue : VK_NULL_HANDLE;
}

uint32_t VulkanContext::primaryGraphicsQueueFamilyIndex() const noexcept {
    if (!primaryDeviceIndex_.has_value()) {
        return 0;
    }
    return devices_[primaryDeviceIndex_.value()]
        .graphicsQueueFamilyIndex.value_or(0);
}

VkRenderPass VulkanContext::primaryRenderPass() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->swapchainRenderPass : VK_NULL_HANDLE;
}

uint32_t VulkanContext::swapchainImageCount() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr
               ? static_cast<uint32_t>(runtime->swapchainImages.size())
               : 0;
}

uint32_t VulkanContext::swapchainMinImageCount() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->swapchainMinImageCount : 0;
}

VkImageView VulkanContext::sceneViewportImageView() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->sceneViewportColorImage.imageView
                              : VK_NULL_HANDLE;
}

VkImageView VulkanContext::gameViewportImageView() const noexcept {
    const auto* runtime = primaryDeviceRuntime();
    return runtime != nullptr ? runtime->gameViewportColorImage.imageView
                              : VK_NULL_HANDLE;
}

}  // namespace engine::vulkan
