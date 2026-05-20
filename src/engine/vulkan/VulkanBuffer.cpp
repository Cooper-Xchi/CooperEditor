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
