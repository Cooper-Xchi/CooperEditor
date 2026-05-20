#include "engine/vulkan/VulkanContextInternal.hpp"

#include <fstream>
#include <stdexcept>

namespace engine::vulkan::detail {

std::vector<char> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file");
    }

    const auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> code(fileSize);
    file.seekg(0);
    file.read(code.data(), static_cast<std::streamsize>(code.size()));

    if (!file) {
        throw std::runtime_error("Failed to read shader file");
    }

    return code;
}

VkShaderModule createShaderModule(VkDevice logicalDevice,
                                  const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    check(vkCreateShaderModule(logicalDevice,
                               &createInfo,
                               nullptr,
                               &shaderModule),
          "vkCreateShaderModule failed");
    return shaderModule;
}

}  // namespace engine::vulkan::detail
