#pragma once

#include <vector>

#include "engine/editor/EditorTypes.hpp"

namespace engine::ui {

struct UiRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct DockedPanelLayout {
    engine::editor::EditorPanelKind kind;
    engine::editor::EditorPanelArea area;
    bool visible = true;
    UiRect rect{};
};

class DockLayout {
public:
    std::vector<DockedPanelLayout> build(
        int framebufferWidth,
        int framebufferHeight,
        const std::vector<engine::editor::EditorPanel>& panels) const;
};

}  // namespace engine::ui
