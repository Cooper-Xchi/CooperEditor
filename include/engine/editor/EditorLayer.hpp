#pragma once

#include <array>
#include <string>
#include <vector>

#include "engine/camera/PerspectiveCamera.hpp"
#include "engine/editor/EditorTypes.hpp"
#include "engine/input/InputState.hpp"
#include "engine/render/Scene.hpp"
#include "engine/ui/DockLayout.hpp"

namespace engine::editor {

class EditorLayer {
public:
    void initialize(const render::Scene& scene);
    void update(const render::Scene& scene,
                const input::InputState& inputState,
                int framebufferWidth,
                int framebufferHeight);

    ActiveViewport activeViewport() const noexcept;
    bool usesSceneCamera() const noexcept;

    camera::PerspectiveCamera& sceneCamera() noexcept;
    const camera::PerspectiveCamera& sceneCamera() const noexcept;

    const std::vector<EditorPanel>& panels() const noexcept;
    const std::vector<ui::DockedPanelLayout>& panelLayout() const noexcept;
    const EditorSelection& selection() const noexcept;
    const std::vector<std::string>& consoleMessages() const noexcept;
    const std::vector<std::string>& hierarchyLines() const noexcept;
    const std::vector<std::string>& inspectorLines() const noexcept;
    const std::vector<std::string>& consoleLines() const noexcept;

    std::string layoutSummary() const;
    std::string controlsSummary() const;
    std::string windowTitle() const;

private:
    bool wasKeyPressed(const input::InputState& inputState, int key) const;
    void syncSelection(const render::Scene& scene);
    void toggleViewport();
    void togglePanel(EditorPanelKind kind);
    void appendConsoleMessage(std::string message);

    bool initialized_ = false;
    ActiveViewport activeViewport_ = ActiveViewport::Scene;
    camera::PerspectiveCamera sceneCamera_{};
    std::vector<EditorPanel> panels_{};
    ui::DockLayout dockLayout_{};
    std::vector<ui::DockedPanelLayout> panelLayout_{};
    EditorSelection selection_{};
    std::vector<std::string> consoleMessages_{};
    std::vector<std::string> hierarchyLines_{};
    std::vector<std::string> inspectorLines_{};
    std::vector<std::string> consoleLines_{};
    std::array<bool, input::InputState::kKeyCount> previousKeysDown_{};
};

}  // namespace engine::editor
