#pragma once

#include "engine/math/MathTypes.hpp"

namespace engine::render {

struct Transform {
    engine::math::Vec3 position{0.0f, 0.0f, 0.0f};
    engine::math::Vec3 rotationDegrees{0.0f, 0.0f, 0.0f};
    engine::math::Vec3 scale{1.0f, 1.0f, 1.0f};

    engine::math::Mat4 matrix() const;
};

}  // namespace engine::render
