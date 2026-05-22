#include "engine/vulkan/VulkanContextInternal.hpp"

#include <cstring>
#include <stdexcept>

namespace engine::vulkan::detail {

uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                        uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t memoryTypeIndex = 0;
         memoryTypeIndex < memoryProperties.memoryTypeCount;
         ++memoryTypeIndex) {
        if ((typeFilter & (1u << memoryTypeIndex)) != 0 &&
            (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
             properties) == properties) {
            return memoryTypeIndex;
        }
    }

    throw std::runtime_error("Failed to find Vulkan memory type");
}

BufferResource createBufferResource(VkPhysicalDevice physicalDevice,
                                    VkDevice logicalDevice,
                                    VkDeviceSize size,
                                    VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags properties) {
    BufferResource resource;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check(vkCreateBuffer(logicalDevice,
                         &bufferCreateInfo,
                         nullptr,
                         &resource.buffer),
          "vkCreateBuffer failed");

    try {
        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(logicalDevice,
                                      resource.buffer,
                                      &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex =
            findMemoryType(physicalDevice,
                           memoryRequirements.memoryTypeBits,
                           properties);

        check(vkAllocateMemory(logicalDevice,
                               &allocateInfo,
                               nullptr,
                               &resource.memory),
              "vkAllocateMemory failed");
        check(vkBindBufferMemory(logicalDevice,
                                 resource.buffer,
                                 resource.memory,
                                 0),
              "vkBindBufferMemory failed");

        if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            check(vkMapMemory(logicalDevice,
                              resource.memory,
                              0,
                              size,
                              0,
                              &resource.mappedData),
                  "vkMapMemory failed");
        }
    } catch (...) {
        if (resource.memory != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice, resource.memory, nullptr);
            resource.memory = VK_NULL_HANDLE;
        }
        if (resource.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(logicalDevice, resource.buffer, nullptr);
            resource.buffer = VK_NULL_HANDLE;
        }
        throw;
    }

    return resource;
}

ImageResource createImageResource(VkPhysicalDevice physicalDevice,
                                  VkDevice logicalDevice,
                                  uint32_t width,
                                  uint32_t height,
                                  VkFormat format,
                                  VkImageUsageFlags usage,
                                  VkImageAspectFlags aspectMask) {
    ImageResource resource;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check(vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &resource.image),
          "vkCreateImage failed");

    try {
        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(logicalDevice,
                                     resource.image,
                                     &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex =
            findMemoryType(physicalDevice,
                           memoryRequirements.memoryTypeBits,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        check(vkAllocateMemory(logicalDevice,
                               &allocateInfo,
                               nullptr,
                               &resource.memory),
              "vkAllocateMemory for image failed");
        check(vkBindImageMemory(logicalDevice,
                                resource.image,
                                resource.memory,
                                0),
              "vkBindImageMemory failed");

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = resource.image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        check(vkCreateImageView(logicalDevice,
                                &imageViewCreateInfo,
                                nullptr,
                                &resource.imageView),
              "vkCreateImageView for image failed");
    } catch (...) {
        destroyImageResource(logicalDevice, resource);
        throw;
    }

    return resource;
}

void destroyImageResource(VkDevice logicalDevice, ImageResource& image) {
    if (image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(logicalDevice, image.imageView, nullptr);
        image.imageView = VK_NULL_HANDLE;
    }
    if (image.image != VK_NULL_HANDLE) {
        vkDestroyImage(logicalDevice, image.image, nullptr);
        image.image = VK_NULL_HANDLE;
    }
    if (image.memory != VK_NULL_HANDLE) {
        vkFreeMemory(logicalDevice, image.memory, nullptr);
        image.memory = VK_NULL_HANDLE;
    }
}

void uploadVertexData(BufferResource& vertexBuffer,
                      const std::vector<engine::render::Vertex>& vertices) {
    if (vertexBuffer.mappedData == nullptr || vertices.empty()) {
        return;
    }

    std::memcpy(vertexBuffer.mappedData,
                vertices.data(),
                sizeof(engine::render::Vertex) * vertices.size());
}

}  // namespace engine::vulkan::detail
