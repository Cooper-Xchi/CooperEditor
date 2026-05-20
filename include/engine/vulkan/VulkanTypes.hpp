#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "engine/math/MathTypes.hpp"

namespace engine::vulkan {

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SwapchainSelection {
    uint32_t imageCount = 0;
    VkSurfaceFormatKHR surfaceFormat{};
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D extent{};
};

struct FrameSyncObjects {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

struct BufferResource {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedData = nullptr;
};

struct CameraUniformData {
    math::Mat4 viewProjection = math::Mat4::identity();
    math::Mat4 model = math::Mat4::identity();
};

struct DeviceInfo {
    bool isPrimaryDevice = false;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    std::vector<VkQueueFamilyProperties> queueFamilies;
    std::vector<bool> presentSupport;
    std::optional<uint32_t> graphicsQueueFamilyIndex;
    std::optional<uint32_t> presentQueueFamilyIndex;
    SwapchainSupportDetails swapchainSupport;
};

struct DeviceRuntime {
    uint32_t drawVertexCount = 0;
    std::string vertexShaderFile;
    std::string fragmentShaderFile;
    CameraUniformData cameraUniform;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    std::optional<SwapchainSelection> swapchainSelection;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<BufferResource> cameraUniformBuffers;
    std::vector<VkDescriptorSet> cameraDescriptorSets;
    BufferResource vertexBuffer;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    bool commandBuffersRecorded = false;
    std::vector<FrameSyncObjects> framesInFlight;
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrameIndex = 0;
    bool frameSubmitted = false;
};

}  // namespace engine::vulkan
