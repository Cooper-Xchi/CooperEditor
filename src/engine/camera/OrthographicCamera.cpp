#include "engine/camera/OrthographicCamera.hpp"

namespace engine::camera {

math::Mat4 OrthographicCamera::projectionMatrix(float aspectRatio) const {
    const float halfHeight = viewportHeight * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return math::orthographic(-halfWidth,
                              halfWidth,
                              -halfHeight,
                              halfHeight,
                              nearClip,
                              farClip);
}

math::Mat4 OrthographicCamera::viewMatrix() const {
    return math::lookAt(position,
                        {position.x, position.y, position.z - 1.0f},
                        {0.0f, 1.0f, 0.0f});
}

}  // namespace engine::camera
