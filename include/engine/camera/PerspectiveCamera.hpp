#pragma once

#include "engine/math/MathTypes.hpp"

namespace engine::camera {

class PerspectiveCamera {
public:
    math::Mat4 projectionMatrix(float aspectRatio) const;
    math::Mat4 viewMatrix() const;

    math::Vec3 forward() const;
    math::Vec3 right() const;
    math::Vec3 up() const;

    math::Vec3 position{0.0f, 0.0f, 2.0f};
    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;
    float verticalFovDegrees = 60.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
};

}  // namespace engine::camera
