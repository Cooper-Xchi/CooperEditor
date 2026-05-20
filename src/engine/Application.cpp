#include "engine/Application.hpp"

#include <chrono>
#include <iostream>

#include <GLFW/glfw3.h>

#include "engine/camera/CameraController.hpp"
#include "engine/camera/OrthographicCamera.hpp"
#include "engine/camera/PerspectiveCamera.hpp"
#include "engine/config/AppConfig.hpp"
#include "engine/config/AppConfigValidation.hpp"
#include "engine/math/MathTypes.hpp"
#include "engine/render/Scene.hpp"
#include "engine/vulkan/VulkanContext.hpp"
#include "engine/vulkan/VulkanReporter.hpp"
#include "engine/window/Window.hpp"

namespace engine {

namespace {

enum class ActiveCameraType {
    Perspective,
    Orthographic,
};

engine::math::Mat4 buildViewProjection(
    const camera::PerspectiveCamera& camera,
    float aspectRatio) {
    auto projection = camera.projectionMatrix(aspectRatio);
    projection.elements[5] *= -1.0f;
    return engine::math::multiply(projection, camera.viewMatrix());
}

engine::math::Mat4 buildViewProjection(
    const camera::OrthographicCamera& camera,
    float aspectRatio) {
    auto projection = camera.projectionMatrix(aspectRatio);
    projection.elements[5] *= -1.0f;
    return engine::math::multiply(projection, camera.viewMatrix());
}

}  // namespace

void Application::run() {
    const auto& config = config::appConfig(); //获取配置
    config::validateAppConfigOrThrow(config);   //确保配置路径都有

    window::Window window;  //窗口实例
    window.initialize();    //窗口初始化

    const auto scene = render::Scene::createDemoForwardScene();
    vulkan::VulkanContext context; //vct 实例
    context.initialize(window, scene); //对window实例化

    camera::PerspectiveCamera perspectiveCamera;
    camera::OrthographicCamera orthographicCamera;
    camera::CameraController cameraController;
    auto activeCameraType = ActiveCameraType::Perspective;

    //是否实时打印信息
    const auto& runtimeConfig = config.runtime;
    if (runtimeConfig.printStartupSummary) {
        vulkan::printContextSummary(context);
    }
    std::cout << "Entering main loop.\n";
    {
        const auto [width, height] = window.framebufferSize();
        const float aspectRatio =
            height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                       : 1.0f;
        const auto modelMatrix = scene.primaryMesh() != nullptr
                                     ? scene.primaryMesh()->transform.matrix()
                                     : engine::math::Mat4::identity();
        context.updateFrameUniform(buildViewProjection(perspectiveCamera,
                                                       aspectRatio),
                                   modelMatrix);
    }
    //进入渲染
    bool firstFrameReported = false;
    auto previousTick = std::chrono::steady_clock::now();

    while (!window.shouldClose()) {
        const auto currentTick = std::chrono::steady_clock::now();
        const float deltaSeconds =
            std::chrono::duration<float>(currentTick - previousTick).count();
        previousTick = currentTick;

        window.pollEvents();
        const auto& inputState = window.inputState();

        if (inputState.keysDown[GLFW_KEY_1]) {
            activeCameraType = ActiveCameraType::Perspective;
        }
        if (inputState.keysDown[GLFW_KEY_2]) {
            activeCameraType = ActiveCameraType::Orthographic;
        }

        if (activeCameraType == ActiveCameraType::Perspective) {
            cameraController.update(perspectiveCamera, inputState, deltaSeconds);
        } else {
            cameraController.update(orthographicCamera,
                                    inputState,
                                    deltaSeconds);
        }

        const auto [width, height] = window.framebufferSize();
        const float aspectRatio =
            height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                       : 1.0f;
        const auto modelMatrix = scene.primaryMesh() != nullptr
                                     ? scene.primaryMesh()->transform.matrix()
                                     : engine::math::Mat4::identity();
        if (activeCameraType == ActiveCameraType::Perspective) {
            context.updateFrameUniform(buildViewProjection(perspectiveCamera,
                                                           aspectRatio),
                                       modelMatrix);
        } else {
            context.updateFrameUniform(buildViewProjection(orthographicCamera,
                                                           aspectRatio),
                                       modelMatrix);
        }

        //检查窗口大小是否发生变化
        if (window.consumeFramebufferResized()) {
            //重建窗口以及各种状态
            context.handleFramebufferResize();
        }
        //绘制第一帧
        const bool submitted = context.drawFrame();
        if (submitted && !firstFrameReported &&
            runtimeConfig.printFirstFrameMessage) {
            vulkan::printFirstFramePresented();
            firstFrameReported = true;
        }
    }
}

}  // namespace engine
