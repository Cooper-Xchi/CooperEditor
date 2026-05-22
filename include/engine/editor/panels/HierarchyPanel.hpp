#pragma once

#include <string>
#include <vector>

#include "engine/editor/EditorTypes.hpp"
#include "engine/render/Scene.hpp"

namespace engine::editor {

class HierarchyPanel {
public:
    std::vector<std::string> buildLines(const render::Scene& scene,
                                        const EditorSelection& selection) const;
};

}  // namespace engine::editor
