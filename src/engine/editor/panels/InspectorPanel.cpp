#include "engine/editor/panels/InspectorPanel.hpp"

#include <sstream>

namespace engine::editor {

namespace {

std::string vec3Line(const char* label, const engine::math::Vec3& value) {
    std::ostringstream stream;
    stream << label << ": " << value.x << ", " << value.y << ", " << value.z;
    return stream.str();
}

}  // namespace

std::vector<std::string> InspectorPanel::buildLines(
    const render::Scene& scene,
    const EditorSelection& selection) const {
    if (!selection.selectedMeshIndex.has_value() ||
        selection.selectedMeshIndex.value() >= scene.meshes().size()) {
        return {"Nothing selected"};
    }

    const auto& mesh = scene.meshes()[selection.selectedMeshIndex.value()];
    return {
        "Mesh " + std::to_string(selection.selectedMeshIndex.value()),
        vec3Line("Position", mesh.transform.position),
        vec3Line("Rotation", mesh.transform.rotationDegrees),
        vec3Line("Scale", mesh.transform.scale),
        "Vertices: " + std::to_string(mesh.geometry.vertexCount),
        "Vertex shader: " + mesh.material.vertexShaderFile,
        "Fragment shader: " + mesh.material.fragmentShaderFile,
    };
}

}  // namespace engine::editor
