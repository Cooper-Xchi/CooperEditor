#pragma once

#include <cstdint>
#include <vector>

namespace engine::render {

enum class PrimitiveTopology {
    TriangleList,
};

struct Vertex {
    float position[2] = {0.0f, 0.0f};
    float color[3] = {1.0f, 1.0f, 1.0f};
};

struct Geometry {
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    uint32_t vertexCount = 0;
    std::vector<Vertex> vertices;
};

}  // namespace engine::render
