#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include "engine/render/Geometry.hpp"
#include "engine/vulkan/VulkanTypes.hpp"

namespace engine::vulkan::detail {

enum class FrameSubmitStatus {
    Submitted,
    RecreateSwapchain,
};

void check(VkResult result, const char* message);

uint32_t queryRuntimeApiVersion();
std::vector<const char*> requiredInstanceExtensions();
std::vector<const char*> requiredDeviceExtensions();

std::vector<VkQueueFamilyProperties> queryQueueFamilies(
    VkPhysicalDevice physicalDevice);
std::optional<uint32_t> findGraphicsQueueFamily(
    const std::vector<VkQueueFamilyProperties>& queueFamilies);
std::vector<bool> queryPresentSupport(VkPhysicalDevice physicalDevice,
                                      VkSurfaceKHR surface,
                                      uint32_t queueFamilyCount);
SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface);
std::optional<uint32_t> findPresentQueueFamily(
    const std::vector<bool>& presentSupport);
SwapchainSelection chooseSwapchainSelection(
    const SwapchainSupportDetails& swapchainSupport,
    VkExtent2D framebufferExtent);

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice,
                             uint32_t graphicsQueueFamilyIndex,
                             uint32_t presentQueueFamilyIndex);
VkQueue acquireQueue(VkDevice logicalDevice, uint32_t queueFamilyIndex);

VkSwapchainKHR createSwapchain(VkDevice logicalDevice,
                               VkSurfaceKHR surface,
                               uint32_t graphicsQueueFamilyIndex,
                               uint32_t presentQueueFamilyIndex,
                               const SwapchainSelection& selection);
std::vector<VkImage> querySwapchainImages(VkDevice logicalDevice,
                                          VkSwapchainKHR swapchain);
std::vector<VkImageView> createSwapchainImageViews(
    VkDevice logicalDevice,
    const std::vector<VkImage>& swapchainImages,
    VkFormat imageFormat);

VkRenderPass createRenderPass(VkDevice logicalDevice,
                              VkFormat colorAttachmentFormat);
std::vector<VkFramebuffer> createFramebuffers(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainImageViews,
    VkExtent2D extent);
VkCommandPool createCommandPool(VkDevice logicalDevice,
                                uint32_t graphicsQueueFamilyIndex);
std::vector<VkCommandBuffer> allocateCommandBuffers(
    VkDevice logicalDevice,
    VkCommandPool commandPool,
    uint32_t commandBufferCount);
void recordCommandBuffers(VkRenderPass renderPass,
                          VkExtent2D extent,
                          const std::vector<VkFramebuffer>& framebuffers,
                          const std::vector<VkCommandBuffer>& commandBuffers,
                          VkPipelineLayout pipelineLayout,
                          VkPipeline graphicsPipeline,
                          VkBuffer vertexBuffer,
                          const std::vector<VkDescriptorSet>& cameraDescriptorSets,
                          uint32_t drawVertexCount);
VkSemaphore createSemaphore(VkDevice logicalDevice);
VkFence createFence(VkDevice logicalDevice);
std::vector<FrameSyncObjects> createFrameSyncObjects(VkDevice logicalDevice,
                                                     uint32_t frameCount);
FrameSubmitStatus submitSingleFrame(VkDevice logicalDevice,
                                    VkQueue graphicsQueue,
                                    VkQueue presentQueue,
                                    VkSwapchainKHR swapchain,
                                    const std::vector<VkCommandBuffer>& commandBuffers,
                                    std::vector<FrameSyncObjects>& framesInFlight,
                                    std::vector<VkFence>& imagesInFlight,
                                    uint32_t& currentFrameIndex);

std::vector<char> readBinaryFile(const std::string& path);
VkShaderModule createShaderModule(VkDevice logicalDevice,
                                  const std::vector<char>& code);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                        uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);
BufferResource createBufferResource(VkPhysicalDevice physicalDevice,
                                    VkDevice logicalDevice,
                                    VkDeviceSize size,
                                    VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags properties);
void uploadVertexData(BufferResource& vertexBuffer,
                      const std::vector<engine::render::Vertex>& vertices);
VkDescriptorSetLayout createCameraDescriptorSetLayout(VkDevice logicalDevice);
VkDescriptorPool createCameraDescriptorPool(VkDevice logicalDevice,
                                            uint32_t descriptorCount);
std::vector<VkDescriptorSet> allocateCameraDescriptorSets(
    VkDevice logicalDevice,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    const std::vector<BufferResource>& cameraUniformBuffers);
VkPipelineLayout createPipelineLayout(VkDevice logicalDevice,
                                      VkDescriptorSetLayout descriptorSetLayout);
VkPipeline createGraphicsPipeline(VkDevice logicalDevice,
                                  VkExtent2D extent,
                                  VkRenderPass renderPass,
                                  VkPipelineLayout pipelineLayout,
                                  const std::string& vertexShaderFile,
                                  const std::string& fragmentShaderFile);

}  // namespace engine::vulkan::detail
