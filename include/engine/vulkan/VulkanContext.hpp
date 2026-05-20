#pragma once

#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include "engine/render/Scene.hpp"
#include "engine/vulkan/VulkanTypes.hpp"

namespace engine::window {
class Window;
}

namespace engine::vulkan {

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void initialize(const engine::window::Window& window,
                    const engine::render::Scene& scene);
    void shutdown();
    bool drawFrame();
    void handleFramebufferResize();
    void updateFrameUniform(const engine::math::Mat4& viewProjection,
                            const engine::math::Mat4& model);

    uint32_t runtimeApiVersion() const noexcept;
    const std::vector<DeviceInfo>& devices() const noexcept;
    const DeviceRuntime* primaryDeviceRuntime() const noexcept;

private:
    void createInstance();
    void createSurface(const engine::window::Window& window);
    void discoverDevices();
    std::vector<VkPhysicalDevice> enumeratePhysicalDevices() const;
    DeviceInfo collectDeviceInfo(VkPhysicalDevice physicalDevice) const;
    void selectPrimaryDevice();
    void createPrimaryDeviceResources();
    void uploadCameraUniforms(DeviceRuntime& runtime);
    void destroyFrameSyncObjects(DeviceRuntime& runtime);
    void destroySwapchainResources(DeviceRuntime& runtime);
    void createSwapchainResources(const DeviceInfo& device,
                                  DeviceRuntime& runtime);
    void recreateSwapchain(const DeviceInfo& device, DeviceRuntime& runtime);
    DeviceRuntime* primaryDeviceRuntime() noexcept;
    bool waitForValidFramebufferSize() const;

    uint32_t runtimeApiVersion_ = VK_API_VERSION_1_0;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    const engine::window::Window* window_ = nullptr;
    const engine::render::Scene* scene_ = nullptr;
    std::optional<uint32_t> primaryDeviceIndex_;
    std::vector<DeviceInfo> devices_;
    std::optional<DeviceRuntime> primaryDeviceRuntime_;
};

}  // namespace engine::vulkan
