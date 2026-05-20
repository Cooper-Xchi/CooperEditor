#pragma once

#include "engine/render/Geometry.hpp"
#include "engine/render/Material.hpp"
#include "engine/render/Transform.hpp"

namespace engine::render {

struct Mesh {
    Geometry geometry;
    Material material;
    Transform transform;
};

}  // namespace engine::render
