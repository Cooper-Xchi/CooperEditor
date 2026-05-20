#pragma once

#include <vector>

#include "engine/render/Mesh.hpp"

namespace engine::render {

class Scene {
public:
    static Scene createDemoForwardScene();

    const Mesh* primaryMesh() const noexcept;
    const std::vector<Mesh>& meshes() const noexcept;

private:
    std::vector<Mesh> meshes_;
};

}  // namespace engine::render
