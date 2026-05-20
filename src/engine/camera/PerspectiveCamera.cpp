#include "engine/camera/PerspectiveCamera.hpp"

#include <cmath>

namespace engine::camera {

namespace {

engine::math::Vec3 directionFromEuler(float yawDegrees, float pitchDegrees) {
    const float yawRadians = engine::math::radians(yawDegrees);
    const float pitchRadians = engine::math::radians(pitchDegrees);

    return engine::math::normalize({
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians),
    });
}

}  // namespace

math::Mat4 PerspectiveCamera::projectionMatrix(float aspectRatio) const {
    return math::perspective(math::radians(verticalFovDegrees),
                             aspectRatio,
                             nearClip,
                             farClip);
}

math::Mat4 PerspectiveCamera::viewMatrix() const {
    return math::lookAt(position, position + forward(), up());
}

math::Vec3 PerspectiveCamera::forward() const {
    return directionFromEuler(yawDegrees, pitchDegrees);
}

math::Vec3 PerspectiveCamera::right() const {
    return math::normalize(math::cross(forward(), {0.0f, 1.0f, 0.0f}));
}

math::Vec3 PerspectiveCamera::up() const {
    return math::normalize(math::cross(right(), forward()));
}

}  // namespace engine::camera
