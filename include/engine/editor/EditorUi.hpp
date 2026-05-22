#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace engine::editor {
class EditorLayer;
}

namespace engine::window {
class Window;
}

namespace engine::vulkan {
class VulkanContext;
}

namespace engine::editor {

class EditorUi {
public:
    EditorUi() = default;
    ~EditorUi();

    EditorUi(const EditorUi&) = delete;
    EditorUi& operator=(const EditorUi&) = delete;

    void initialize(const engine::window::Window& window,
                    const engine::vulkan::VulkanContext& context);
    void shutdown();
    void beginFrame();
    void build(const EditorLayer& editorLayer);
    void render(VkCommandBuffer commandBuffer) const;
    void syncMinImageCount(uint32_t minImageCount);
    void syncSceneViewportTexture(VkImageView imageView);
    void syncGameViewportTexture(VkImageView imageView);

private:
    void drawHierarchyPanel(const EditorLayer& editorLayer);
    void drawInspectorPanel(const EditorLayer& editorLayer);
    void drawSceneViewPanel(const EditorLayer& editorLayer);
    void drawGameViewPanel(const EditorLayer& editorLayer);
    void drawWorkspacePanel(const EditorLayer& editorLayer);

    bool initialized_ = false;
    uint32_t minImageCount_ = 0;
    VkImageView sceneViewportImageView_ = VK_NULL_HANDLE;
    VkImageView gameViewportImageView_ = VK_NULL_HANDLE;
    VkDescriptorSet sceneViewportTexture_ = VK_NULL_HANDLE;
    VkDescriptorSet gameViewportTexture_ = VK_NULL_HANDLE;
    float leftPaneRatio_ = 0.20f;
    float rightPaneRatio_ = 0.22f;
    float bottomPaneRatio_ = 0.26f;
    float gameViewRatio_ = 0.30f;
};

}  // namespace engine::editor
