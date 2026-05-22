#include "engine/ui/DockLayout.hpp"

namespace engine::ui {

std::vector<DockedPanelLayout> DockLayout::build(
    int framebufferWidth,
    int framebufferHeight,
    const std::vector<engine::editor::EditorPanel>& panels) const {
    const int width = framebufferWidth > 0 ? framebufferWidth : 1;
    const int height = framebufferHeight > 0 ? framebufferHeight : 1;

    constexpr int kOuterPadding = 6;
    constexpr int kGutter = 4;
    constexpr int kTopToolbarHeight = 34;

    const int contentX = kOuterPadding;
    const int contentY = kTopToolbarHeight + kOuterPadding;
    const int contentWidth = width - kOuterPadding * 2;
    const int contentHeight = height - contentY - kOuterPadding;

    const int leftWidth = contentWidth * 18 / 100;
    const int rightWidth = contentWidth * 22 / 100;
    const int bottomHeight = contentHeight * 26 / 100;
    const int centerWidth = contentWidth - leftWidth - rightWidth - kGutter * 2;
    const int centerHeight = contentHeight - bottomHeight - kGutter;
    const int centerTopHeight = centerHeight * 58 / 100;
    const int centerBottomHeight = centerHeight - centerTopHeight - kGutter;
    const int bottomPanelWidth = (contentWidth - kGutter) / 2;

    std::vector<DockedPanelLayout> layout;
    layout.reserve(panels.size());

    int centerStackIndex = 0;
    int bottomStackIndex = 0;

    for (const auto& panel : panels) {
        UiRect rect{};
        switch (panel.area) {
        case engine::editor::EditorPanelArea::Left:
            rect = {contentX, contentY, leftWidth, centerHeight};
            break;
        case engine::editor::EditorPanelArea::Right:
            rect = {contentX + leftWidth + kGutter + centerWidth + kGutter,
                    contentY,
                    rightWidth,
                    centerHeight};
            break;
        case engine::editor::EditorPanelArea::Center:
            rect = {contentX + leftWidth + kGutter,
                    centerStackIndex == 0 ? contentY
                                          : contentY + centerTopHeight + kGutter,
                    centerWidth,
                    centerStackIndex == 0 ? centerTopHeight : centerBottomHeight};
            ++centerStackIndex;
            break;
        case engine::editor::EditorPanelArea::Bottom:
            rect = {contentX,
                    contentY + centerHeight + kGutter,
                    contentWidth,
                    bottomHeight};
            ++bottomStackIndex;
            break;
        }

        layout.push_back({panel.kind, panel.area, panel.visible, rect});
    }

    return layout;
}

}  // namespace engine::ui
