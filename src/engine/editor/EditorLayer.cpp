#include "engine/editor/EditorLayer.hpp"

#include <sstream>

#include <GLFW/glfw3.h>

#include "engine/editor/panels/ConsolePanel.hpp"
#include "engine/editor/panels/HierarchyPanel.hpp"
#include "engine/editor/panels/InspectorPanel.hpp"

namespace engine::editor {

namespace {

constexpr std::string_view panelTitle(EditorPanelKind kind) {
    switch (kind) {
    case EditorPanelKind::Hierarchy:
        return "Hierarchy";
    case EditorPanelKind::Inspector:
        return "Inspector";
    case EditorPanelKind::SceneView:
        return "Scene View";
    case EditorPanelKind::GameView:
        return "Game View";
    case EditorPanelKind::Project:
        return "Project";
    case EditorPanelKind::Console:
        return "Console";
    }
    return "Panel";
}

std::string viewportLabel(ActiveViewport viewport) {
    return viewport == ActiveViewport::Scene ? "Scene" : "Game";
}

}  // namespace

void EditorLayer::initialize(const render::Scene& scene) {
    if (initialized_) {
        return;
    }

    sceneCamera_.position = {0.0f, 1.5f, 4.0f};
    sceneCamera_.yawDegrees = -90.0f;
    sceneCamera_.pitchDegrees = -18.0f;

    panels_ = {
        {EditorPanelKind::Hierarchy, panelTitle(EditorPanelKind::Hierarchy), EditorPanelArea::Left, true},
        {EditorPanelKind::Inspector, panelTitle(EditorPanelKind::Inspector), EditorPanelArea::Right, true},
        {EditorPanelKind::SceneView, panelTitle(EditorPanelKind::SceneView), EditorPanelArea::Center, true},
        {EditorPanelKind::GameView, panelTitle(EditorPanelKind::GameView), EditorPanelArea::Center, true},
        {EditorPanelKind::Project, panelTitle(EditorPanelKind::Project), EditorPanelArea::Bottom, true},
        {EditorPanelKind::Console, panelTitle(EditorPanelKind::Console), EditorPanelArea::Bottom, true},
    };

    syncSelection(scene);
    appendConsoleMessage("Editor layout bootstrapped.");
    appendConsoleMessage("Active viewport: Scene.");
    hierarchyLines_ = HierarchyPanel{}.buildLines(scene, selection_);
    inspectorLines_ = InspectorPanel{}.buildLines(scene, selection_);
    consoleLines_ = ConsolePanel{}.buildLines(consoleMessages_);
    initialized_ = true;
}

void EditorLayer::update(const render::Scene& scene,
                         const input::InputState& inputState,
                         int framebufferWidth,
                         int framebufferHeight) {
    if (!initialized_) {
        initialize(scene);
    }

    syncSelection(scene);

    if (wasKeyPressed(inputState, GLFW_KEY_TAB)) {
        toggleViewport();
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F1)) {
        togglePanel(EditorPanelKind::Hierarchy);
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F2)) {
        togglePanel(EditorPanelKind::Inspector);
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F3)) {
        togglePanel(EditorPanelKind::SceneView);
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F4)) {
        togglePanel(EditorPanelKind::GameView);
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F5)) {
        togglePanel(EditorPanelKind::Project);
    }
    if (wasKeyPressed(inputState, GLFW_KEY_F6)) {
        togglePanel(EditorPanelKind::Console);
    }

    panelLayout_ = dockLayout_.build(framebufferWidth, framebufferHeight, panels_);
    hierarchyLines_ = HierarchyPanel{}.buildLines(scene, selection_);
    inspectorLines_ = InspectorPanel{}.buildLines(scene, selection_);
    consoleLines_ = ConsolePanel{}.buildLines(consoleMessages_);
    previousKeysDown_ = inputState.keysDown;
}

ActiveViewport EditorLayer::activeViewport() const noexcept {
    return activeViewport_;
}

bool EditorLayer::usesSceneCamera() const noexcept {
    return activeViewport_ == ActiveViewport::Scene;
}

camera::PerspectiveCamera& EditorLayer::sceneCamera() noexcept {
    return sceneCamera_;
}

const camera::PerspectiveCamera& EditorLayer::sceneCamera() const noexcept {
    return sceneCamera_;
}

const std::vector<EditorPanel>& EditorLayer::panels() const noexcept {
    return panels_;
}

const std::vector<ui::DockedPanelLayout>& EditorLayer::panelLayout() const noexcept {
    return panelLayout_;
}

const EditorSelection& EditorLayer::selection() const noexcept {
    return selection_;
}

const std::vector<std::string>& EditorLayer::consoleMessages() const noexcept {
    return consoleMessages_;
}

const std::vector<std::string>& EditorLayer::hierarchyLines() const noexcept {
    return hierarchyLines_;
}

const std::vector<std::string>& EditorLayer::inspectorLines() const noexcept {
    return inspectorLines_;
}

const std::vector<std::string>& EditorLayer::consoleLines() const noexcept {
    return consoleLines_;
}

std::string EditorLayer::layoutSummary() const {
    std::ostringstream stream;
    stream << "Editor layout: Left[Hierarchy] Right[Inspector] "
              "Center[Scene View|Game View] Bottom[Project|Console]";
    return stream.str();
}

std::string EditorLayer::controlsSummary() const {
    return "Editor controls: Tab switches Scene/Game viewport, F1-F6 toggle panels.";
}

std::string EditorLayer::windowTitle() const {
    std::ostringstream stream;
    stream << "Engine Editor | " << viewportLabel(activeViewport_) << " View";

    if (selection_.selectedMeshIndex.has_value()) {
        stream << " | Selected Mesh " << selection_.selectedMeshIndex.value();
    } else {
        stream << " | Nothing Selected";
    }

    size_t visiblePanelCount = 0;
    for (const auto& panel : panels_) {
        if (panel.visible) {
            ++visiblePanelCount;
        }
    }
    stream << " | Panels " << visiblePanelCount << "/" << panels_.size();
    return stream.str();
}

bool EditorLayer::wasKeyPressed(const input::InputState& inputState,
                                int key) const {
    if (key < 0 || key >= input::InputState::kKeyCount) {
        return false;
    }

    const size_t index = static_cast<size_t>(key);
    return inputState.keysDown[index] && !previousKeysDown_[index];
}

void EditorLayer::syncSelection(const render::Scene& scene) {
    if (!selection_.selectedMeshIndex.has_value()) {
        if (!scene.meshes().empty()) {
            selection_.selectedMeshIndex = 0;
        }
        return;
    }

    if (*selection_.selectedMeshIndex >= scene.meshes().size()) {
        selection_.selectedMeshIndex.reset();
        if (!scene.meshes().empty()) {
            selection_.selectedMeshIndex = 0;
        }
    }
}

void EditorLayer::toggleViewport() {
    activeViewport_ = activeViewport_ == ActiveViewport::Scene
                          ? ActiveViewport::Game
                          : ActiveViewport::Scene;
    appendConsoleMessage("Active viewport switched to " +
                         viewportLabel(activeViewport_) + ".");
}

void EditorLayer::togglePanel(EditorPanelKind kind) {
    for (auto& panel : panels_) {
        if (panel.kind != kind) {
            continue;
        }

        panel.visible = !panel.visible;
        appendConsoleMessage(std::string(panel.title) +
                             (panel.visible ? " shown." : " hidden."));
        return;
    }
}

void EditorLayer::appendConsoleMessage(std::string message) {
    consoleMessages_.push_back(std::move(message));
    constexpr size_t kMaxConsoleMessages = 32;
    if (consoleMessages_.size() > kMaxConsoleMessages) {
        consoleMessages_.erase(consoleMessages_.begin());
    }
}

}  // namespace engine::editor
