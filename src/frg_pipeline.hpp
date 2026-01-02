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
    FrgPipeline(
        FrgDevice &device, const std::string &vertFilePath,
        const std::string &fragFilePath, const PipelineConfigInfo &configInfo
    );

    ~FrgPipeline();

    // Delete copy constructor and copy assignment operator
    FrgPipeline(const FrgPipeline &) = delete;
    FrgPipeline &operator=(const FrgPipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);

  private:
    static std::vector<char> readFile(const std::string &filePath);
    void createGraphicsPipeline(
        const std::string &vertFilePath, const std::string &fragFilePath,
        const PipelineConfigInfo &configInfo
    );

    void create_compute_pipeline(const std::string &shader_file_path);

    void createShaderModule(
        const std::vector<char> &code, VkShaderModule *shaderModule
    );

    FrgDevice &frgDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkShaderModule compShaderModule;
};
} // namespace frg