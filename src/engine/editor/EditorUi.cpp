#include "engine/editor/EditorUi.hpp"

#include <algorithm>
#include <stdexcept>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "engine/editor/EditorLayer.hpp"
#include "engine/editor/EditorTypes.hpp"
#include "engine/vulkan/VulkanContext.hpp"
#include "engine/window/Window.hpp"

namespace engine::editor {

namespace {

void applyUnityStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 3.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(8.0f, 6.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
    style.IndentSpacing = 16.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.40f, 0.64f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.24f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.40f, 0.64f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.21f, 0.37f, 0.52f, 0.75f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.43f, 0.61f, 0.86f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.40f, 0.64f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.43f, 0.61f, 0.90f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.24f, 0.55f, 0.85f, 0.70f);
}

const char* panelWindowName(EditorPanelKind kind) {
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

void drawPanelHeader(const char* title) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
    ImGui::BeginChild(title, ImVec2(0.0f, 28.0f), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(title);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void drawTextLines(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        ImGui::TextUnformatted(line.c_str());
    }
}

void drawViewportPanel(const char* title,
                       bool active,
                       const camera::PerspectiveCamera& camera,
                       VkDescriptorSet viewportTexture) {
    ImGui::TextUnformatted(active ? "Active viewport" : "Inactive viewport");
    ImGui::Separator();
    ImGui::Text("Camera position: %.2f %.2f %.2f",
                camera.position.x,
                camera.position.y,
                camera.position.z);
    ImGui::Text("Yaw/Pitch: %.2f / %.2f", camera.yawDegrees, camera.pitchDegrees);
    ImGui::TextUnformatted(title);
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportTexture != VK_NULL_HANDLE &&
        viewportSize.x > 8.0f &&
        viewportSize.y > 8.0f) {
        ImGui::Image(viewportTexture,
                     viewportSize,
                     ImVec2(0.0f, 1.0f),
                     ImVec2(1.0f, 0.0f));
    } else {
        ImGui::Button("Viewport Surface Pending", ImVec2(-1.0f, 220.0f));
    }
}

bool drawSplitter(const char* id,
                  bool vertical,
                  float length,
                  float thickness,
                  float* size1,
                  float* size2,
                  float minSize1,
                  float minSize2) {
    ImVec2 backupPos = ImGui::GetCursorPos();
    if (vertical) {
        ImGui::SetCursorPosY(backupPos.y);
        ImGui::InvisibleButton(id, ImVec2(length, thickness));
    } else {
        ImGui::SetCursorPosX(backupPos.x);
        ImGui::InvisibleButton(id, ImVec2(thickness, length));
    }

    if (ImGui::IsItemActive()) {
        const float mouseDelta = vertical ? ImGui::GetIO().MouseDelta.y
                                          : ImGui::GetIO().MouseDelta.x;
        float newSize1 = *size1 + mouseDelta;
        float newSize2 = *size2 - mouseDelta;
        if (newSize1 >= minSize1 && newSize2 >= minSize2) {
            *size1 = newSize1;
            *size2 = newSize2;
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Border);
    drawList->AddRectFilled(min, max, color);
    return ImGui::IsItemActive();
}

void drawBottomWorkspace(const EditorLayer& editorLayer) {
    if (ImGui::BeginTabBar("BottomWorkspaceTabs")) {
        if (ImGui::BeginTabItem("Project")) {
            ImGui::TextUnformatted("Assets browser placeholder");
            ImGui::Separator();
            ImGui::TextUnformatted("Next step: expose project files and import state.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Console")) {
            drawTextLines(editorLayer.consoleLines());
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

}  // namespace

EditorUi::~EditorUi() {
    shutdown();
}

void EditorUi::initialize(const engine::window::Window& window,
                          const engine::vulkan::VulkanContext& context) {
    shutdown();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    applyUnityStyle();

    if (!ImGui_ImplGlfw_InitForVulkan(window.nativeHandle(), true)) {
        throw std::runtime_error("ImGui GLFW backend initialization failed");
    }

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = context.runtimeApiVersion();
    initInfo.Instance = context.instanceHandle();
    initInfo.PhysicalDevice = context.primaryPhysicalDevice();
    initInfo.Device = context.primaryLogicalDevice();
    initInfo.QueueFamily = context.primaryGraphicsQueueFamilyIndex();
    initInfo.Queue = context.primaryGraphicsQueue();
    initInfo.DescriptorPool = VK_NULL_HANDLE;
    initInfo.DescriptorPoolSize = 64;
    initInfo.MinImageCount = context.swapchainMinImageCount();
    initInfo.ImageCount = context.swapchainImageCount();
    initInfo.PipelineInfoMain.RenderPass = context.primaryRenderPass();
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("ImGui Vulkan backend initialization failed");
    }

    minImageCount_ = context.swapchainMinImageCount();
    initialized_ = true;
}

void EditorUi::shutdown() {
    if (!initialized_) {
        return;
    }

    if (sceneViewportTexture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(sceneViewportTexture_);
        sceneViewportTexture_ = VK_NULL_HANDLE;
    }
    if (gameViewportTexture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(gameViewportTexture_);
        gameViewportTexture_ = VK_NULL_HANDLE;
    }
    sceneViewportImageView_ = VK_NULL_HANDLE;
    gameViewportImageView_ = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
    minImageCount_ = 0;
}

void EditorUi::beginFrame() {
    if (!initialized_) {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorUi::build(const EditorLayer& editorLayer) {
    if (!initialized_) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostWindowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EngineEditorHost", nullptr, hostWindowFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        ImGui::TextUnformatted("Engine Editor");
        ImGui::Separator();
        ImGui::TextUnformatted(editorLayer.controlsSummary().c_str());
        ImGui::EndMenuBar();
    }

    constexpr float splitterSize = 6.0f;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float minLeft = 180.0f;
    const float minRight = 220.0f;
    const float minBottom = 140.0f;
    const float minCenter = 280.0f;
    const float minTopCenter = 180.0f;
    const float minGame = 120.0f;

    float leftWidth = available.x * leftPaneRatio_;
    float rightWidth = available.x * rightPaneRatio_;
    float centerWidth = available.x - leftWidth - rightWidth - splitterSize * 2.0f;
    if (centerWidth < minCenter) {
        centerWidth = minCenter;
        rightWidth = std::max(minRight, available.x - leftWidth - centerWidth - splitterSize * 2.0f);
    }

    float bottomHeight = available.y * bottomPaneRatio_;
    float topHeight = available.y - bottomHeight - splitterSize;
    if (topHeight < minTopCenter) {
        topHeight = minTopCenter;
        bottomHeight = available.y - topHeight - splitterSize;
    }

    float gameHeight = topHeight * gameViewRatio_;
    float sceneHeight = topHeight - gameHeight - splitterSize;
    if (sceneHeight < minTopCenter) {
        sceneHeight = minTopCenter;
        gameHeight = topHeight - sceneHeight - splitterSize;
    }

    ImGui::BeginChild("EditorContentRoot", available, false, ImGuiWindowFlags_NoScrollbar);

    ImGui::BeginChild("LeftColumn", ImVec2(leftWidth, topHeight), true);
    drawHierarchyPanel(editorLayer);
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    drawSplitter("LeftCenterSplitter", false, topHeight, splitterSize, &leftWidth, &centerWidth, minLeft, minCenter);
    leftPaneRatio_ = leftWidth / std::max(1.0f, available.x);

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("CenterColumn", ImVec2(centerWidth, topHeight), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::BeginChild("SceneSection", ImVec2(0.0f, sceneHeight), true);
    drawSceneViewPanel(editorLayer);
    ImGui::EndChild();

    drawSplitter("SceneGameSplitter", true, centerWidth, splitterSize, &sceneHeight, &gameHeight, minTopCenter, minGame);
    gameViewRatio_ = gameHeight / std::max(1.0f, topHeight);

    ImGui::BeginChild("GameSection", ImVec2(0.0f, 0.0f), true);
    drawGameViewPanel(editorLayer);
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    drawSplitter("CenterRightSplitter", false, topHeight, splitterSize, &centerWidth, &rightWidth, minCenter, minRight);
    rightPaneRatio_ = rightWidth / std::max(1.0f, available.x);

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("RightColumn", ImVec2(0.0f, topHeight), true);
    drawInspectorPanel(editorLayer);
    ImGui::EndChild();

    drawSplitter("TopBottomSplitter", true, available.x, splitterSize, &topHeight, &bottomHeight, minTopCenter, minBottom);
    bottomPaneRatio_ = bottomHeight / std::max(1.0f, available.y);

    ImGui::BeginChild("BottomWorkspace", ImVec2(0.0f, 0.0f), true);
    drawWorkspacePanel(editorLayer);
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
}

void EditorUi::render(VkCommandBuffer commandBuffer) const {
    if (!initialized_) {
        return;
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void EditorUi::syncMinImageCount(uint32_t minImageCount) {
    if (!initialized_ || minImageCount_ == minImageCount) {
        return;
    }

    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
    minImageCount_ = minImageCount;
}

void EditorUi::syncSceneViewportTexture(VkImageView imageView) {
    if (!initialized_ || sceneViewportImageView_ == imageView) {
        return;
    }

    if (sceneViewportTexture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(sceneViewportTexture_);
        sceneViewportTexture_ = VK_NULL_HANDLE;
    }

    sceneViewportImageView_ = imageView;
    if (sceneViewportImageView_ != VK_NULL_HANDLE) {
        sceneViewportTexture_ =
            ImGui_ImplVulkan_AddTexture(sceneViewportImageView_,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void EditorUi::syncGameViewportTexture(VkImageView imageView) {
    if (!initialized_ || gameViewportImageView_ == imageView) {
        return;
    }

    if (gameViewportTexture_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(gameViewportTexture_);
        gameViewportTexture_ = VK_NULL_HANDLE;
    }

    gameViewportImageView_ = imageView;
    if (gameViewportImageView_ != VK_NULL_HANDLE) {
        gameViewportTexture_ =
            ImGui_ImplVulkan_AddTexture(gameViewportImageView_,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void EditorUi::drawHierarchyPanel(const EditorLayer& editorLayer) {
    drawPanelHeader("Hierarchy");
    drawTextLines(editorLayer.hierarchyLines());
}

void EditorUi::drawInspectorPanel(const EditorLayer& editorLayer) {
    drawPanelHeader("Inspector");
    drawTextLines(editorLayer.inspectorLines());
}

void EditorUi::drawSceneViewPanel(const EditorLayer& editorLayer) {
    drawPanelHeader("Scene View");
    drawViewportPanel("Scene camera preview",
                      editorLayer.activeViewport() == ActiveViewport::Scene,
                      editorLayer.sceneCamera(),
                      sceneViewportTexture_);
}

void EditorUi::drawGameViewPanel(const EditorLayer& editorLayer) {
    drawPanelHeader("Game View");
    drawViewportPanel("Game camera preview",
                      editorLayer.activeViewport() == ActiveViewport::Game,
                      editorLayer.sceneCamera(),
                      gameViewportTexture_);
}

void EditorUi::drawWorkspacePanel(const EditorLayer& editorLayer) {
    drawPanelHeader("Workspace");
    drawBottomWorkspace(editorLayer);
}

}  // namespace engine::editor
