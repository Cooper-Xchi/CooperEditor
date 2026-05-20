#pragma once

#include "engine/camera/OrthographicCamera.hpp"
#include "engine/camera/PerspectiveCamera.hpp"
#include "engine/input/InputState.hpp"

namespace engine::camera {

struct CameraControllerSettings {
    float moveSpeed = 3.0f;
    float lookSensitivity = 0.08f;
    float orthoPanSpeed = 3.0f;
    float orthoZoomSpeed = 2.0f;
};

class CameraController {
public:
    explicit CameraController(CameraControllerSettings settings = {});

    void update(PerspectiveCamera& camera,
                const input::InputState& inputState,
                float deltaSeconds) const;
    void update(OrthographicCamera& camera,
                const input::InputState& inputState,
                float deltaSeconds) const;

private:
    CameraControllerSettings settings_;
};

}  // namespace engine::camera
