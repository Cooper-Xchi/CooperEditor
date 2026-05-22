#pragma once

#include <optional>
#include <string_view>

namespace engine::editor {

enum class EditorPanelArea {
    Left,
    Right,
    Center,
    Bottom,
};

enum class EditorPanelKind {
    Hierarchy,
    Inspector,
    SceneView,
    GameView,
    Project,
    Console,
};

enum class ActiveViewport {
    Scene,
    Game,
};

struct EditorPanel {
    EditorPanelKind kind;
    std::string_view title;
    EditorPanelArea area;
    bool visible = true;
};

struct EditorSelection {
    std::optional<size_t> selectedMeshIndex;
};

}  // namespace engine::editor
