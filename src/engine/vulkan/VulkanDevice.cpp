#include "engine/vulkan/VulkanContext.hpp"

#include <stdexcept>

#include "engine/vulkan/VulkanContextInternal.hpp"
#include "engine/window/Window.hpp"

namespace engine::vulkan::detail {

std::vector<VkQueueFamilyProperties> queryQueueFamilies(
    VkPhysicalDevice physicalDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount,
                                             queueFamilies.data());
    return queueFamilies;
}

std::optional<uint32_t> findGraphicsQueueFamily(
    const std::vector<VkQueueFamilyProperties>& queueFamilies) {
    for (uint32_t familyIndex = 0;
         familyIndex < static_cast<uint32_t>(queueFamilies.size());
         ++familyIndex) {
        if ((queueFamilies[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) !=
            0) {
            return familyIndex;
        }
    }

    return std::nullopt;
}

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice,
                             uint32_t graphicsQueueFamilyIndex,
                             uint32_t presentQueueFamilyIndex) {
    const float queuePriority = 1.0f;
    const auto deviceExtensions = requiredDeviceExtensions();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(2);

    VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueCreateInfo);

    if (presentQueueFamilyIndex != graphicsQueueFamilyIndex) {
        VkDeviceQueueCreateInfo presentQueueCreateInfo{};
        presentQueueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
        presentQueueCreateInfo.queueCount = 1;
        presentQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(presentQueueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice logicalDevice = VK_NULL_HANDLE;
    check(vkCreateDevice(physicalDevice,
                         &deviceCreateInfo,
                         nullptr,
                         &logicalDevice),
          "vkCreateDevice failed");
    return logicalDevice;
}

VkQueue acquireQueue(VkDevice logicalDevice, uint32_t queueFamilyIndex) {
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &queue);
    return queue;
}

}  // namespace engine::vulkan::detail

namespace engine::vulkan {

namespace {

bool isDeviceSuitable(const DeviceInfo& device) {
    return device.graphicsQueueFamilyIndex.has_value() &&
           device.presentQueueFamilyIndex.has_value() &&
           !device.swapchainSupport.formats.empty() &&
           !device.swapchainSupport.presentModes.empty();
}

}  // namespace

std::vector<VkPhysicalDevice> VulkanContext::enumeratePhysicalDevices() const {
    uint32_t deviceCount = 0;
    detail::check(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr),
                  "vkEnumeratePhysicalDevices count failed");

    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan physical devices found");
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    detail::check(vkEnumeratePhysicalDevices(instance_,
                                             &deviceCount,
                                             physicalDevices.data()),
                  "vkEnumeratePhysicalDevices list failed");
    return physicalDevices;
}

DeviceInfo VulkanContext::collectDeviceInfo(
    VkPhysicalDevice physicalDevice) const {
    DeviceInfo deviceInfo;
    deviceInfo.physicalDevice = physicalDevice;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceInfo.properties);

    deviceInfo.queueFamilies = detail::queryQueueFamilies(physicalDevice);
    deviceInfo.presentSupport =
        detail::queryPresentSupport(physicalDevice,
                                    surface_,
                                    static_cast<uint32_t>(
                                        deviceInfo.queueFamilies.size()));
    deviceInfo.swapchainSupport =
        detail::querySwapchainSupport(physicalDevice, surface_);
    deviceInfo.graphicsQueueFamilyIndex =
        detail::findGraphicsQueueFamily(deviceInfo.queueFamilies);
    deviceInfo.presentQueueFamilyIndex =
        detail::findPresentQueueFamily(deviceInfo.presentSupport);

    return deviceInfo;
}

void VulkanContext::selectPrimaryDevice() {
    primaryDeviceIndex_.reset();

    for (uint32_t deviceIndex = 0;
         deviceIndex < static_cast<uint32_t>(devices_.size());
         ++deviceIndex) {
        devices_[deviceIndex].isPrimaryDevice = false;
        if (!primaryDeviceIndex_.has_value() &&
            isDeviceSuitable(devices_[deviceIndex])) {
            primaryDeviceIndex_ = deviceIndex;
            devices_[deviceIndex].isPrimaryDevice = true;
        }
    }

    if (!primaryDeviceIndex_.has_value()) {
        throw std::runtime_error("No suitable Vulkan physical device found");
    }
}

void VulkanContext::createPrimaryDeviceResources() {
    if (scene_ == nullptr || scene_->primaryMesh() == nullptr) {
        throw std::runtime_error("Forward scene has no primary mesh");
    }

    const auto& primaryMesh = *scene_->primaryMesh();
    if (primaryMesh.geometry.vertices.empty()) {
        throw std::runtime_error("Primary mesh has no vertices");
    }
    const auto& deviceInfo = devices_[primaryDeviceIndex_.value()];
    if (!deviceInfo.graphicsQueueFamilyIndex.has_value() ||
        !deviceInfo.presentQueueFamilyIndex.has_value()) {
        return;
    }

    primaryDeviceRuntime_.emplace();
    auto& runtime = primaryDeviceRuntime_.value();
    runtime.logicalDevice =
        detail::createLogicalDevice(deviceInfo.physicalDevice,
                                    deviceInfo.graphicsQueueFamilyIndex.value(),
                                    deviceInfo.presentQueueFamilyIndex.value());
    runtime.graphicsQueue =
        detail::acquireQueue(runtime.logicalDevice,
                             deviceInfo.graphicsQueueFamilyIndex.value());
    runtime.presentQueue =
        detail::acquireQueue(runtime.logicalDevice,
                             deviceInfo.presentQueueFamilyIndex.value());
    runtime.drawVertexCount = primaryMesh.geometry.vertexCount;
    runtime.vertexShaderFile = primaryMesh.material.vertexShaderFile;
    runtime.fragmentShaderFile = primaryMesh.material.fragmentShaderFile;
    runtime.vertexBuffer =
        detail::createBufferResource(deviceInfo.physicalDevice,
                                     runtime.logicalDevice,
                                     sizeof(engine::render::Vertex) *
                                         primaryMesh.geometry.vertices.size(),
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    detail::uploadVertexData(runtime.vertexBuffer, primaryMesh.geometry.vertices);
    runtime.descriptorSetLayout =
        detail::createCameraDescriptorSetLayout(runtime.logicalDevice);
    runtime.pipelineLayout =
        detail::createPipelineLayout(runtime.logicalDevice,
                                     runtime.descriptorSetLayout);

    if (!deviceInfo.swapchainSupport.formats.empty() &&
        !deviceInfo.swapchainSupport.presentModes.empty()) {
        createSwapchainResources(deviceInfo, runtime);
    }
}

void VulkanContext::discoverDevices() {
    const auto physicalDevices = enumeratePhysicalDevices();
    devices_.clear();
    devices_.reserve(physicalDevices.size());

    for (const auto physicalDevice : physicalDevices) {
        devices_.push_back(collectDeviceInfo(physicalDevice));
    }

    selectPrimaryDevice();
    createPrimaryDeviceResources();
}

}  // namespace engine::vulkan
