#pragma once

namespace engine::vulkan {

class VulkanContext;

void printContextSummary(const VulkanContext& context);
void printFirstFramePresented();

}  // namespace engine::vulkan
