#include "engine/camera/CameraController.hpp"

#include <iostream>
#include <ostream>
#include <GLFW/glfw3.h>

namespace engine::camera {

CameraController::CameraController(CameraControllerSettings settings)
    : settings_(settings) {}

void CameraController::update(PerspectiveCamera& camera,
                              const input::InputState& inputState,
                              float deltaSeconds) const {
    const float moveDistance = settings_.moveSpeed * deltaSeconds;

    if (inputState.keysDown[GLFW_KEY_W]) {
        std::cout<<"W"<<std::endl;
        camera.position = camera.position + camera.forward() * moveDistance;
    }
    if (inputState.keysDown[GLFW_KEY_S]) {
        std::cout<<"S"<<std::endl;
        camera.position = camera.position - camera.forward() * moveDistance;
    }
    if (inputState.keysDown[GLFW_KEY_D]) {
        std::cout<<"D"<<std::endl;
        camera.position = camera.position + camera.right() * moveDistance;
    }
    if (inputState.keysDown[GLFW_KEY_A]) {
        std::cout<<"A"<<std::endl;
        camera.position = camera.position - camera.right() * moveDistance;
    }
    if (inputState.keysDown[GLFW_KEY_E]) {
        std::cout<<"E"<<std::endl;
        camera.position.y += moveDistance;
    }
    if (inputState.keysDown[GLFW_KEY_Q]) {
        std::cout<<"Q"<<std::endl;
        camera.position.y -= moveDistance;
    }

    if (inputState.mouseButtonsDown[GLFW_MOUSE_BUTTON_RIGHT]) {
        std::cout<<"mouseDeltaX"<<inputState.mouseDeltaX<<"and"<<inputState.mouseDeltaY<<std::endl;
        camera.yawDegrees +=
            static_cast<float>(inputState.mouseDeltaX) * settings_.lookSensitivity;
        camera.pitchDegrees -=
            static_cast<float>(inputState.mouseDeltaY) * settings_.lookSensitivity;
        camera.pitchDegrees = math::clamp(camera.pitchDegrees, -89.0f, 89.0f);
    }
}

void CameraController::update(OrthographicCamera& camera,
                              const input::InputState& inputState,
                              float deltaSeconds) const {
    const float panDistance = settings_.orthoPanSpeed * deltaSeconds;

    if (inputState.keysDown[GLFW_KEY_W]) {
        camera.position.y += panDistance;
    }
    if (inputState.keysDown[GLFW_KEY_S]) {
        camera.position.y -= panDistance;
    }
    if (inputState.keysDown[GLFW_KEY_D]) {
        camera.position.x += panDistance;
    }
    if (inputState.keysDown[GLFW_KEY_A]) {
        camera.position.x -= panDistance;
    }

    camera.viewportHeight -=
        static_cast<float>(inputState.scrollDeltaY) * settings_.orthoZoomSpeed;
    if (inputState.keysDown[GLFW_KEY_Q]) {
        camera.viewportHeight += panDistance;
    }
    if (inputState.keysDown[GLFW_KEY_E]) {
        camera.viewportHeight -= panDistance;
    }
    camera.viewportHeight = math::clamp(camera.viewportHeight, 0.5f, 50.0f);
}

}  // namespace engine::camera
