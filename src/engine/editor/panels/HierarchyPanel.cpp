#include "engine/editor/panels/HierarchyPanel.hpp"

namespace engine::editor {

std::vector<std::string> HierarchyPanel::buildLines(
    const render::Scene& scene,
    const EditorSelection& selection) const {
    std::vector<std::string> lines;
    lines.reserve(scene.meshes().size() + 1);
    lines.push_back("Scene");

    for (size_t index = 0; index < scene.meshes().size(); ++index) {
        const bool selected = selection.selectedMeshIndex.has_value() &&
                              selection.selectedMeshIndex.value() == index;
        lines.push_back(std::string(selected ? "> " : "  ") + "Mesh " +
                        std::to_string(index));
    }

    return lines;
}

}  // namespace engine::editor
