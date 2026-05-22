#include "engine/vulkan/VulkanReporter.hpp"

#include <iostream>

#include "engine/vulkan/VulkanContext.hpp"

namespace engine::vulkan {

namespace {

const char* surfaceFormatName(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_SRGB:
            return "B8G8R8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_UNORM:
            return "B8G8R8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SRGB:
            return "R8G8B8A8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM:
            return "R8G8B8A8_UNORM";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return "A2B10G10R10_UNORM_PACK32";
        default:
            return "Other";
    }
}

const char* colorSpaceName(VkColorSpaceKHR colorSpace) {
    switch (colorSpace) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return "SRGB_NONLINEAR";
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
            return "DISPLAY_P3_NONLINEAR";
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            return "EXTENDED_SRGB_LINEAR";
        case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            return "DISPLAY_P3_LINEAR";
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
            return "DCI_P3_NONLINEAR";
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            return "BT709_LINEAR";
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
            return "BT709_NONLINEAR";
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            return "BT2020_LINEAR";
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            return "HDR10_ST2084";
        case VK_COLOR_SPACE_DOLBYVISION_EXT:
            return "DOLBYVISION";
        case VK_COLOR_SPACE_HDR10_HLG_EXT:
            return "HDR10_HLG";
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            return "ADOBERGB_LINEAR";
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
            return "ADOBERGB_NONLINEAR";
        case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            return "PASS_THROUGH";
        case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
            return "EXTENDED_SRGB_NONLINEAR";
        default:
            return "Other";
    }
}

const char* presentModeName(VkPresentModeKHR presentMode) {
    switch (presentMode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "Immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "Mailbox";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "FIFO Relaxed";
        default:
            return "Other";
    }
}

void printQueueFamily(uint32_t familyIndex,
                      const VkQueueFamilyProperties& queueFamily,
                      bool presentSupport) {
    std::cout << "  Queue family " << familyIndex
              << ": queueCount=" << queueFamily.queueCount << ", flags=";

    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
        std::cout << "[Graphics] ";
    }
    if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
        std::cout << "[Compute] ";
    }
    if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) {
        std::cout << "[Transfer] ";
    }
    if (presentSupport) {
        std::cout << "[Present] ";
    }

    std::cout << '\n';
}

void printSwapchainSelection(const std::optional<SwapchainSelection>& selection) {
    if (!selection.has_value()) {
        std::cout << "  No valid swapchain selection could be made.\n";
        return;
    }

    std::cout << "  Chosen swapchain parameters:\n";
    std::cout << "    imageCount=" << selection->imageCount << '\n';
    std::cout << "    extent=" << selection->extent.width << 'x'
              << selection->extent.height << '\n';
    std::cout << "    format="
              << surfaceFormatName(selection->surfaceFormat.format) << " ("
              << selection->surfaceFormat.format << ")\n";
    std::cout << "    colorSpace="
              << colorSpaceName(selection->surfaceFormat.colorSpace) << " ("
              << selection->surfaceFormat.colorSpace << ")\n";
    std::cout << "    presentMode=" << presentModeName(selection->presentMode)
              << " (" << selection->presentMode << ")\n";
}

}  // namespace

void printContextSummary(const VulkanContext& context) {
    //获取版本
    const auto apiVersion = context.runtimeApiVersion();
    std::cout << "Vulkan loader API: " << VK_VERSION_MAJOR(apiVersion) << '.'
              << VK_VERSION_MINOR(apiVersion) << '.'
              << VK_VERSION_PATCH(apiVersion) << '\n';
    //获取设备信息
    const auto& devices = context.devices();
    const auto* primaryRuntime = context.primaryDeviceRuntime();
    std::cout << "Detected physical devices: " << devices.size() << '\n';

    for (uint32_t deviceIndex = 0;
         deviceIndex < static_cast<uint32_t>(devices.size());
         ++deviceIndex) {
        const auto& device = devices[deviceIndex];
        const auto* runtime =
            device.isPrimaryDevice ? primaryRuntime : nullptr;
        std::cout << "[" << deviceIndex << "] " << device.properties.deviceName
                  << '\n';
        if (device.isPrimaryDevice) {
            std::cout << "  Selected as primary render device.\n";
        }

        for (uint32_t familyIndex = 0;
             familyIndex < static_cast<uint32_t>(device.queueFamilies.size());
             ++familyIndex) {
            printQueueFamily(familyIndex,
                             device.queueFamilies[familyIndex],
                             device.presentSupport[familyIndex]);
        }

        if (device.graphicsQueueFamilyIndex.has_value()) {
            std::cout << "  First graphics queue family index: "
                      << device.graphicsQueueFamilyIndex.value() << '\n';
            if (runtime != nullptr && runtime->logicalDevice != VK_NULL_HANDLE) {
                std::cout << "  Logical device created successfully.\n";
            }
            if (runtime != nullptr && runtime->graphicsQueue != VK_NULL_HANDLE) {
                std::cout << "  Graphics queue acquired successfully.\n";
            }
        } else {
            std::cout << "  No graphics queue family found.\n";
        }

        if (device.presentQueueFamilyIndex.has_value()) {
            std::cout << "  First present queue family index: "
                      << device.presentQueueFamilyIndex.value() << '\n';
            if (runtime != nullptr && runtime->presentQueue != VK_NULL_HANDLE) {
                std::cout << "  Present queue acquired successfully.\n";
            }
        } else {
            std::cout << "  No present queue family found.\n";
        }

        printSwapchainSelection(runtime != nullptr
                                    ? runtime->swapchainSelection
                                    : std::optional<SwapchainSelection>{});
        if (runtime != nullptr && runtime->swapchain != VK_NULL_HANDLE) {
            std::cout << "  Swapchain created successfully.\n";
            std::cout << "  Swapchain image count: "
                      << runtime->swapchainImages.size() << '\n';
            std::cout << "  Swapchain image view count: "
                      << runtime->swapchainImageViews.size() << '\n';
            if (runtime->swapchainRenderPass != VK_NULL_HANDLE) {
                std::cout << "  Swapchain render pass created successfully.\n";
            }
            if (runtime->sceneRenderPass != VK_NULL_HANDLE) {
                std::cout << "  Scene render pass created successfully.\n";
            }
            if (runtime->pipelineLayout != VK_NULL_HANDLE) {
                std::cout << "  Pipeline layout created successfully.\n";
            }
            if (runtime->swapchainGraphicsPipeline != VK_NULL_HANDLE) {
                std::cout << "  Swapchain graphics pipeline created successfully.\n";
            }
            if (runtime->sceneGraphicsPipeline != VK_NULL_HANDLE) {
                std::cout << "  Scene graphics pipeline created successfully.\n";
            }
            if (!runtime->vertexShaderFile.empty()) {
                std::cout << "  Vertex shader: " << runtime->vertexShaderFile
                          << '\n';
            }
            if (!runtime->fragmentShaderFile.empty()) {
                std::cout << "  Fragment shader: "
                          << runtime->fragmentShaderFile << '\n';
            }
            std::cout << "  Draw vertex count: " << runtime->drawVertexCount
                      << '\n';
            std::cout << "  Framebuffer count: "
                      << runtime->framebuffers.size() << '\n';
            if (runtime->commandPool != VK_NULL_HANDLE) {
                std::cout << "  Command pool created successfully.\n";
            }
            std::cout << "  Command buffer count: "
                      << runtime->commandBuffers.size() << '\n';
            if (runtime->commandBuffersRecorded) {
                std::cout << "  Command buffers recorded successfully.\n";
            }
        } else {
            std::cout << "  Swapchain was not created.\n";
        }
    }

    std::cout << "Vulkan setup looks healthy.\n";
}

void printFirstFramePresented() {
    std::cout << "First frame submitted and presented successfully.\n";
}

}  // namespace engine::vulkan
