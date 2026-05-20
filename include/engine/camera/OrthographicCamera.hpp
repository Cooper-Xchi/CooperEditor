#pragma once

#include "engine/math/MathTypes.hpp"

namespace engine::camera {

class OrthographicCamera {
public:
    math::Mat4 projectionMatrix(float aspectRatio) const;
    math::Mat4 viewMatrix() const;

    math::Vec3 position{0.0f, 0.0f, 2.0f};
    float viewportHeight = 4.0f;
    float nearClip = -10.0f;
    float farClip = 10.0f;
};

}  // namespace engine::camera
