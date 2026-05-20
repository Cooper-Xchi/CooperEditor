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

void VulkanContext::uploadCameraUniforms(DeviceRuntime& runtime) {
    for (auto& uniformBuffer : runtime.cameraUniformBuffers) {
        if (uniformBuffer.mappedData != nullptr) {
            std::memcpy(uniformBuffer.mappedData,
                        &runtime.cameraUniform,
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

    if (runtime.descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(runtime.logicalDevice, runtime.descriptorPool, nullptr);
        runtime.descriptorPool = VK_NULL_HANDLE;
    }
    runtime.cameraDescriptorSets.clear();

    for (auto& uniformBuffer : runtime.cameraUniformBuffers) {
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
    runtime.cameraUniformBuffers.clear();

    if (runtime.commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(runtime.logicalDevice, runtime.commandPool, nullptr);
        runtime.commandPool = VK_NULL_HANDLE;
    }
    runtime.commandBuffers.clear();
    runtime.commandBuffersRecorded = false;

    if (runtime.graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(runtime.logicalDevice, runtime.graphicsPipeline, nullptr);
        runtime.graphicsPipeline = VK_NULL_HANDLE;
    }

    for (auto& framebuffer : runtime.framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(runtime.logicalDevice, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }
    runtime.framebuffers.clear();

    if (runtime.renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(runtime.logicalDevice, runtime.renderPass, nullptr);
        runtime.renderPass = VK_NULL_HANDLE;
    }

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
    runtime.renderPass =
        detail::createRenderPass(runtime.logicalDevice,
                                 selection.surfaceFormat.format);
    runtime.graphicsPipeline =
        detail::createGraphicsPipeline(runtime.logicalDevice,
                                       selection.extent,
                                       runtime.renderPass,
                                       runtime.pipelineLayout,
                                       runtime.vertexShaderFile,
                                       runtime.fragmentShaderFile);
    runtime.cameraUniformBuffers.clear();
    runtime.cameraUniformBuffers.reserve(runtime.swapchainImages.size());
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(runtime.swapchainImages.size());
         ++index) {
        runtime.cameraUniformBuffers.push_back(
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
                                               runtime.cameraUniformBuffers.size()));
    runtime.cameraDescriptorSets =
        detail::allocateCameraDescriptorSets(runtime.logicalDevice,
                                             runtime.descriptorPool,
                                             runtime.descriptorSetLayout,
                                             runtime.cameraUniformBuffers);
    uploadCameraUniforms(runtime);
    runtime.framebuffers =
        detail::createFramebuffers(runtime.logicalDevice,
                                   runtime.renderPass,
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
    detail::recordCommandBuffers(runtime.renderPass,
                                 selection.extent,
                                 runtime.framebuffers,
                                 runtime.commandBuffers,
                                 runtime.pipelineLayout,
                                 runtime.graphicsPipeline,
                                 runtime.vertexBuffer.buffer,
                                 runtime.cameraDescriptorSets,
                                 runtime.drawVertexCount);
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

bool VulkanContext::drawFrame() {
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
    const auto submitStatus =
        detail::submitSingleFrame(runtime->logicalDevice,
                                  runtime->graphicsQueue,
                                  runtime->presentQueue,
                                  runtime->swapchain,
                                  runtime->commandBuffers,
                                  runtime->framesInFlight,
                                  runtime->imagesInFlight,
                                  runtime->currentFrameIndex);
    if (submitStatus == detail::FrameSubmitStatus::RecreateSwapchain) {
        recreateSwapchain(device, *runtime);
        return false;
    }
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

void VulkanContext::updateFrameUniform(const engine::math::Mat4& viewProjection,
                                       const engine::math::Mat4& model) {
    auto* runtime = primaryDeviceRuntime();
    if (runtime == nullptr) {
        return;
    }

    runtime->cameraUniform.viewProjection = viewProjection;
    runtime->cameraUniform.model = model;
    uploadCameraUniforms(*runtime);
}

uint32_t VulkanContext::runtimeApiVersion() const noexcept {
    return runtimeApiVersion_;
}

const std::vector<DeviceInfo>& VulkanContext::devices() const noexcept {
    return devices_;
}

}  // namespace engine::vulkan
