#include "engine/config/AppConfig.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace engine::config {

namespace {

    //获取配置文件路径
std::filesystem::path findConfigFilePath() {
    auto current = std::filesystem::current_path(); //获取项目路径   /Users/chenhongchi/Desktop/Engine

    while (true) {
        const auto candidate = current / "config" / "app.ini";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }

        if (current == current.root_path()) {
            return {};
        }
        current = current.parent_path();
    }
}

std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(),
                value.end());
    return value;
}

bool parseBool(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    return fallback;
}

int parseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

uint32_t parseUint32(const std::string& value, uint32_t fallback) {
    try {
        return static_cast<uint32_t>(std::stoul(value));
    } catch (...) {
        return fallback;
    }
}

float parseFloat(const std::string& value, float fallback) {
    try {
        return std::stof(value);
    } catch (...) {
        return fallback;
    }
}

void applyWindowValue(WindowConfig& config,
                      const std::string& key,
                      const std::string& value) {
    if (key == "title") {
        config.title = value;
    } else if (key == "width") {
        config.width = parseInt(value, config.width);
    } else if (key == "height") {
        config.height = parseInt(value, config.height);
    } else if (key == "resizable") {
        config.resizable = parseBool(value, config.resizable);
    }
}

void applyVulkanValue(VulkanAppConfig& config,
                      const std::string& key,
                      const std::string& value) {
    if (key == "application_name") {
        config.applicationName = value;
    } else if (key == "application_version_major") {
        config.applicationVersionMajor =
            parseUint32(value, config.applicationVersionMajor);
    } else if (key == "application_version_minor") {
        config.applicationVersionMinor =
            parseUint32(value, config.applicationVersionMinor);
    } else if (key == "application_version_patch") {
        config.applicationVersionPatch =
            parseUint32(value, config.applicationVersionPatch);
    } else if (key == "engine_name") {
        config.engineName = value;
    } else if (key == "engine_version_major") {
        config.engineVersionMajor =
            parseUint32(value, config.engineVersionMajor);
    } else if (key == "engine_version_minor") {
        config.engineVersionMinor =
            parseUint32(value, config.engineVersionMinor);
    } else if (key == "engine_version_patch") {
        config.engineVersionPatch =
            parseUint32(value, config.engineVersionPatch);
    }
}

void applyRenderValue(RenderConfig& config,
                      const std::string& key,
                      const std::string& value) {
    if (key == "clear_r") {
        config.clearColor[0] = parseFloat(value, config.clearColor[0]);
    } else if (key == "clear_g") {
        config.clearColor[1] = parseFloat(value, config.clearColor[1]);
    } else if (key == "clear_b") {
        config.clearColor[2] = parseFloat(value, config.clearColor[2]);
    } else if (key == "clear_a") {
        config.clearColor[3] = parseFloat(value, config.clearColor[3]);
    }
}

void applyRuntimeValue(RuntimeConfig& config,
                       const std::string& key,
                       const std::string& value) {
    if (key == "print_startup_summary") {
        config.printStartupSummary =
            parseBool(value, config.printStartupSummary);
    } else if (key == "print_first_frame_message") {
        config.printFirstFrameMessage =
            parseBool(value, config.printFirstFrameMessage);
    }
}

void applyRenderLoopValue(RenderLoopConfig& config,
                          const std::string& key,
                          const std::string& value) {
    if (key == "prefer_vsync") {
        config.preferVSync = parseBool(value, config.preferVSync);
    } else if (key == "prefer_mailbox_present_mode") {
        config.preferMailboxPresentMode =
            parseBool(value, config.preferMailboxPresentMode);
    }
}

void applyPathsValue(PathsConfig& config,
                     const std::string& key,
                     const std::string& value) {
    if (key == "shader_root") {
        config.shaderRoot = value;
    } else if (key == "asset_root") {
        config.assetRoot = value;
    } else if (key == "cache_root") {
        config.cacheRoot = value;
    }
}

std::string resolvePathString(const std::filesystem::path& baseDirectory,
                              const std::string& value) {
    const std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }
    return (baseDirectory / path).lexically_normal().string();
}

void resolveRelativePaths(AppConfig& config,
                          const std::filesystem::path& baseDirectory) {
    //赋值shader、资产、缓存路径
    config.paths.shaderRoot =
        resolvePathString(baseDirectory, config.paths.shaderRoot);
    config.paths.assetRoot =
        resolvePathString(baseDirectory, config.paths.assetRoot);
    config.paths.cacheRoot =
        resolvePathString(baseDirectory, config.paths.cacheRoot);
}

AppConfig loadConfigFromFile(const AppConfig& defaults) {
    AppConfig config = defaults;    //默认从外部先传递默认配置
    const auto configFilePath = findConfigFilePath(); //获取配置文件路径

    //如果路径空
    if (configFilePath.empty()) {
        //解析相对路径
        resolveRelativePaths(config, std::filesystem::current_path());
        return config;
    }

    //尝试打开配置文件
    std::ifstream input(configFilePath);
    //如果失败
    if (!input.is_open()) {
        //解析相对路径
        resolveRelativePaths(config, std::filesystem::current_path());
        return config;
    }

    //取根目录
    const auto projectRoot = configFilePath.parent_path().parent_path();

    std::string line;
    std::string section;

    //按行读取配置文件
    while (std::getline(input, line)) {

        //查找注释位置
        const auto commentPos = line.find_first_of("#;");

        //如果这一行包含注释，那就删掉注释以及之后的内容
        if (commentPos != std::string::npos) {
            line.erase(commentPos);
        }
        //去掉行首和行尾空白
        line = trim(line);

        //处理完就空了就跳过
        if (line.empty()) {
            continue;
        }

        //如果是节标题，如[window]
        if (line.front() == '[' && line.back() == ']') {
            //提取window
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        //如果没找到=，说明不合法跳过
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        //提取键值对
        const auto key = trim(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));

        //根据节标题分别分发对应模块
        if (section == "window") {
            applyWindowValue(config.window, key, value);
        } else if (section == "vulkan") {
            applyVulkanValue(config.vulkan, key, value);
        } else if (section == "render") {
            applyRenderValue(config.render, key, value);
        } else if (section == "runtime") {
            applyRuntimeValue(config.runtime, key, value);
        } else if (section == "render_loop") {
            applyRenderLoopValue(config.renderLoop, key, value);
        } else if (section == "paths") {
            applyPathsValue(config.paths, key, value);
        }
    }
    //解析相关路径
    resolveRelativePaths(config, projectRoot);
    return config;
}

}  // namespace

WindowConfig defaultWindowConfig() {
    WindowConfig config;
    config.title = "Engine";
    config.width = 1280;
    config.height = 720;
    config.resizable = false;
    return config;
}

VulkanAppConfig defaultVulkanAppConfig() {
    VulkanAppConfig config;
    config.applicationName = "Engine";
    config.applicationVersionMajor = 1;
    config.applicationVersionMinor = 0;
    config.applicationVersionPatch = 0;
    config.engineName = "LearningEngine";
    config.engineVersionMajor = 1;
    config.engineVersionMinor = 0;
    config.engineVersionPatch = 0;
    return config;
}

RenderConfig defaultRenderConfig() {
    RenderConfig config;
    config.clearColor = {0.12f, 0.42f, 0.55f, 1.0f};
    return config;
}

RuntimeConfig defaultRuntimeConfig() {
    RuntimeConfig config;
    config.printStartupSummary = true;
    config.printFirstFrameMessage = true;
    return config;
}

RenderLoopConfig defaultRenderLoopConfig() {
    RenderLoopConfig config;
    config.preferVSync = true;
    config.preferMailboxPresentMode = true;
    return config;
}

PathsConfig defaultPathsConfig() {
    PathsConfig config;
    config.shaderRoot = "shaders";
    config.assetRoot = "assets";
    config.cacheRoot = "cache";
    return config;
}

    //提供了一个默认配置，值由该文件内部传递
AppConfig defaultAppConfig() {
    AppConfig config;
    config.window = defaultWindowConfig();
    config.vulkan = defaultVulkanAppConfig();
    config.render = defaultRenderConfig();
    config.runtime = defaultRuntimeConfig();
    config.renderLoop = defaultRenderLoopConfig();
    config.paths = defaultPathsConfig();
    return config;
}

const AppConfig& appConfig() {
    static const AppConfig config = loadConfigFromFile(defaultAppConfig());  //通过文件获取配置
    return config;
}

}  // namespace engine::config
