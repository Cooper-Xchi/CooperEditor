#include "engine/vulkan/VulkanContext.hpp"

#include <stdexcept>

#include <GLFW/glfw3.h>

#include "engine/config/AppConfig.hpp"
#include "engine/window/Window.hpp"
#include "engine/vulkan/VulkanContextInternal.hpp"

namespace engine::vulkan::detail {

void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}

uint32_t queryRuntimeApiVersion() {
    uint32_t version = VK_API_VERSION_1_0;

#if VK_HEADER_VERSION >= 70
    const auto enumerateInstanceVersion =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (enumerateInstanceVersion != nullptr) {
        enumerateInstanceVersion(&version);
    }
#endif

    return version;
}

std::vector<const char*> requiredInstanceExtensions() {
    std::vector<const char*> extensions;
    uint32_t glfwExtensionCount = 0;
    const auto* glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t index = 0; index < glfwExtensionCount; ++index) {
        extensions.push_back(glfwExtensions[index]);
    }
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    return extensions;
}

std::vector<const char*> requiredDeviceExtensions() {
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

}  // namespace engine::vulkan::detail

namespace engine::vulkan {

void VulkanContext::createInstance() {
    const auto extensions = detail::requiredInstanceExtensions();//获取设备拓展指令
    const auto& config = engine::config::appConfig().vulkan;//获取配置
    //设置app信息
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = config.applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(config.applicationVersionMajor,
                                                 config.applicationVersionMinor,
                                                 config.applicationVersionPatch);
    appInfo.pEngineName = config.engineName.c_str();
    appInfo.engineVersion = VK_MAKE_VERSION(config.engineVersionMajor,
                                            config.engineVersionMinor,
                                            config.engineVersionPatch);
    appInfo.apiVersion = runtimeApiVersion_;
    //设置实例信息
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames =
        extensions.empty() ? nullptr : extensions.data();

#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    //创建并检查instance，成功复制instance
    detail::check(vkCreateInstance(&createInfo, nullptr, &instance_),
                  "vkCreateInstance failed");
}

void VulkanContext::createSurface(const engine::window::Window& window) {

    //给instance一个surface，这样来翻译vulkan作用在glfw。同时给到_surface
    detail::check(glfwCreateWindowSurface(instance_,
                                          window.nativeHandle(),
                                          nullptr,
                                          &surface_),
                  "glfwCreateWindowSurface failed");
}

}  // namespace engine::vulkan
