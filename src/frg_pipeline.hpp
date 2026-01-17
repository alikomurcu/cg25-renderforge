#pragma once

#include "frg_device.hpp"

// std
#include <string>
#include <vector>

namespace frg {
struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo &) = delete;
    PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

    // Vertex input (optional - if empty, uses default Vertex class)
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    // Multiple color blend attachments (for MRT)
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};

    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class FrgPipeline {
  public:
    FrgPipeline(FrgDevice &device, const std::string &vertFilePath, const std::string &fragFilePath,
                const PipelineConfigInfo &configInfo);

    ~FrgPipeline();

    // Delete copy constructor and copy assignment operator
    FrgPipeline(const FrgPipeline &) = delete;
    FrgPipeline &operator=(const FrgPipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);

  private:
    static std::vector<char> readFile(const std::string &filePath);
    void createGraphicsPipeline(const std::string &vertFilePath, const std::string &fragFilePath,
                                const PipelineConfigInfo &configInfo);

    void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    FrgDevice &frgDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};
} // namespace frg