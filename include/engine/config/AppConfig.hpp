#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace engine::config {

// Window system settings. These control the native GLFW window.
struct WindowConfig {
    std::string title;
    int width;
    int height;
    bool resizable;
};

// Vulkan application identity shown to the driver and validation tools.
struct VulkanAppConfig {
    std::string applicationName;
    uint32_t applicationVersionMajor;
    uint32_t applicationVersionMinor;
    uint32_t applicationVersionPatch;
    std::string engineName;
    uint32_t engineVersionMajor;
    uint32_t engineVersionMinor;
    uint32_t engineVersionPatch;
};

// Render defaults used before higher-level renderer systems exist.
struct RenderConfig {
    std::array<float, 4> clearColor;
};

// Runtime diagnostics and teaching-oriented output switches.
struct RuntimeConfig {
    bool printStartupSummary;
    bool printFirstFrameMessage;
};

// Frame pacing and present-mode preferences.
struct RenderLoopConfig {
    bool preferVSync;
    bool preferMailboxPresentMode;
};

// Project-level directories used by future resource systems.
struct PathsConfig {
    std::string shaderRoot;
    std::string assetRoot;
    std::string cacheRoot;
};

// Top-level engine configuration shared across modules.
struct AppConfig {
    WindowConfig window;
    VulkanAppConfig vulkan;
    RenderConfig render;
    RuntimeConfig runtime;
    RenderLoopConfig renderLoop;
    PathsConfig paths;
};

WindowConfig defaultWindowConfig();
VulkanAppConfig defaultVulkanAppConfig();
RenderConfig defaultRenderConfig();
RuntimeConfig defaultRuntimeConfig();
RenderLoopConfig defaultRenderLoopConfig();
PathsConfig defaultPathsConfig();
AppConfig defaultAppConfig();

const AppConfig& appConfig();

}  // namespace engine::config
