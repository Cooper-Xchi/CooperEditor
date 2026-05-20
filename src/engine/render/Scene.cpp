#include "engine/render/Scene.hpp"

namespace engine::render {

Scene Scene::createDemoForwardScene() {
    Scene scene;

    Mesh triangleMesh;
    triangleMesh.geometry.topology = PrimitiveTopology::TriangleList;
    triangleMesh.geometry.vertices = {
        Vertex{{0.0f, -0.5f}, {1.0f, 0.2f, 0.2f}},
        Vertex{{0.5f, 0.5f}, {0.2f, 1.0f, 0.2f}},
        Vertex{{-0.5f, 0.5f}, {0.2f, 0.4f, 1.0f}},
    };
    triangleMesh.geometry.vertexCount =
        static_cast<uint32_t>(triangleMesh.geometry.vertices.size());
    triangleMesh.material.vertexShaderFile = "triangle.vert.spv";
    triangleMesh.material.fragmentShaderFile = "triangle.frag.spv";
    triangleMesh.transform.position = {0.3f, 0.0f, 0.0f};
    triangleMesh.transform.rotationDegrees = {0.0f, 0.0f, -18.0f};

    scene.meshes_.push_back(triangleMesh);
    return scene;
}

const Mesh* Scene::primaryMesh() const noexcept {
    if (meshes_.empty()) {
        return nullptr;
    }
    return &meshes_.front();
}

const std::vector<Mesh>& Scene::meshes() const noexcept {
    return meshes_;
}

}  // namespace engine::render
