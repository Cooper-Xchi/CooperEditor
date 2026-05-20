#include "engine/vulkan/VulkanContext.hpp"

#include <algorithm>

#include "engine/config/AppConfig.hpp"
#include "engine/vulkan/VulkanContextInternal.hpp"

namespace engine::vulkan::detail {

std::vector<bool> queryPresentSupport(VkPhysicalDevice physicalDevice,
                                      VkSurfaceKHR surface,
                                      uint32_t queueFamilyCount) {
    std::vector<bool> presentSupport(queueFamilyCount, false);
    for (uint32_t familyIndex = 0; familyIndex < queueFamilyCount;
         ++familyIndex) {
        VkBool32 isSupported = VK_FALSE;
        check(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice,
                                                   familyIndex,
                                                   surface,
                                                   &isSupported),
              "vkGetPhysicalDeviceSurfaceSupportKHR failed");
        presentSupport[familyIndex] = (isSupported == VK_TRUE);
    }

    return presentSupport;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface) {
    SwapchainSupportDetails details;

    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
                                                    surface,
                                                    &details.capabilities),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");

    uint32_t formatCount = 0;
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                               surface,
                                               &formatCount,
                                               nullptr),
          "vkGetPhysicalDeviceSurfaceFormatsKHR count failed");
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                                   surface,
                                                   &formatCount,
                                                   details.formats.data()),
              "vkGetPhysicalDeviceSurfaceFormatsKHR list failed");
    }

    uint32_t presentModeCount = 0;
    check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
                                                    surface,
                                                    &presentModeCount,
                                                    nullptr),
          "vkGetPhysicalDeviceSurfacePresentModesKHR count failed");
    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        check(vkGetPhysicalDeviceSurfacePresentModesKHR(
                  physicalDevice,
                  surface,
                  &presentModeCount,
                  details.presentModes.data()),
              "vkGetPhysicalDeviceSurfacePresentModesKHR list failed");
    }

    return details;
}

std::optional<uint32_t> findPresentQueueFamily(
    const std::vector<bool>& presentSupport) {
    for (uint32_t familyIndex = 0;
         familyIndex < static_cast<uint32_t>(presentSupport.size());
         ++familyIndex) {
        if (presentSupport[familyIndex]) {
            return familyIndex;
        }
    }

    return std::nullopt;
}

namespace {

VkSurfaceFormatKHR chooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

VkPresentModeKHR choosePresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
    const auto& renderLoopConfig = engine::config::appConfig().renderLoop;

    if (!renderLoopConfig.preferVSync) {
        for (const auto availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return availablePresentMode;
            }
        }
    }

    if (renderLoopConfig.preferMailboxPresentMode) {
        for (const auto availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
    }

    for (const auto availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                        VkExtent2D framebufferExtent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    VkExtent2D extent{};
    extent.width =
        std::clamp(framebufferExtent.width,
                   capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    extent.height =
        std::clamp(framebufferExtent.height,
                   capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);
    return extent;
}

uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    return imageCount;
}

}  // namespace

SwapchainSelection chooseSwapchainSelection(
    const SwapchainSupportDetails& swapchainSupport,
    VkExtent2D framebufferExtent) {
    SwapchainSelection selection;
    selection.imageCount = chooseImageCount(swapchainSupport.capabilities);
    selection.surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
    selection.presentMode = choosePresentMode(swapchainSupport.presentModes);
    selection.extent =
        chooseExtent(swapchainSupport.capabilities, framebufferExtent);
    return selection;
}

VkSwapchainKHR createSwapchain(VkDevice logicalDevice,
                               VkSurfaceKHR surface,
                               uint32_t graphicsQueueFamilyIndex,
                               uint32_t presentQueueFamilyIndex,
                               const SwapchainSelection& selection) {
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = selection.imageCount;
    createInfo.imageFormat = selection.surfaceFormat.format;
    createInfo.imageColorSpace = selection.surfaceFormat.colorSpace;
    createInfo.imageExtent = selection.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = {
        graphicsQueueFamilyIndex,
        presentQueueFamilyIndex,
    };

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = selection.presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    check(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain),
          "vkCreateSwapchainKHR failed");
    return swapchain;
}

std::vector<VkImage> querySwapchainImages(VkDevice logicalDevice,
                                          VkSwapchainKHR swapchain) {
    uint32_t imageCount = 0;
    check(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr),
          "vkGetSwapchainImagesKHR count failed");

    std::vector<VkImage> images(imageCount);
    check(vkGetSwapchainImagesKHR(logicalDevice,
                                  swapchain,
                                  &imageCount,
                                  images.data()),
          "vkGetSwapchainImagesKHR list failed");
    return images;
}

std::vector<VkImageView> createSwapchainImageViews(
    VkDevice logicalDevice,
    const std::vector<VkImage>& swapchainImages,
    VkFormat imageFormat) {
    std::vector<VkImageView> imageViews;
    imageViews.reserve(swapchainImages.size());

    try {
        for (const auto image : swapchainImages) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = imageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkImageView imageView = VK_NULL_HANDLE;
            check(vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView),
                  "vkCreateImageView failed");
            imageViews.push_back(imageView);
        }
    } catch (...) {
        for (auto& imageView : imageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(logicalDevice, imageView, nullptr);
                imageView = VK_NULL_HANDLE;
            }
        }
        throw;
    }

    return imageViews;
}

}  // namespace engine::vulkan::detail
