#include "engine/render/Transform.hpp"

namespace engine::render {

engine::math::Mat4 Transform::matrix() const {
    const auto translation =
        engine::math::translation(position.x, position.y, position.z);
    const auto rotation = engine::math::multiply(
        engine::math::multiply(
            engine::math::rotationZ(engine::math::radians(rotationDegrees.z)),
            engine::math::rotationY(engine::math::radians(rotationDegrees.y))),
        engine::math::rotationX(engine::math::radians(rotationDegrees.x)));
    const auto scaleMatrix =
        engine::math::scaleMatrix(scale.x, scale.y, scale.z);
    return engine::math::multiply(translation,
                                  engine::math::multiply(rotation, scaleMatrix));
}

}  // namespace engine::render
