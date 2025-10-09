#pragma once

#include <VulkanRenderer/VulkanRendererDLL.h>
#include <VulkanRenderer/Core/VkTypes.h>
#include <VulkanRenderer/Core/VkSwapChain.h>

#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/UniquePtr.h>

class nsVkInstance;
class nsVkDevice;
class nsVkCommandContext;

struct nsVulkanRendererCreateInfo
{
  void* m_pWindowHandle = nullptr;
  nsUInt32 m_uiWidth = 0;
  nsUInt32 m_uiHeight = 0;
  bool m_bEnableValidation = true;
};

struct nsVulkanInstanceData
{
  nsMat4 m_ModelMatrix = nsMat4::MakeIdentity();
  nsColor m_Color = nsColor::White;
  bool m_bSleeping = false;
};

class NS_VULKANRENDERER_DLL nsVulkanRenderer
{
public:
  nsVulkanRenderer();
  ~nsVulkanRenderer();

  nsResult Initialize(const nsVulkanRendererCreateInfo& createInfo);
  void Deinitialize();
  nsResult RenderFrame();
  void SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight);
  void UpdateScene(const nsMat4& mViewProjection, nsArrayPtr<const nsVulkanInstanceData> instances);
  bool IsInitialized() const { return m_pDevice != nullptr; }

private:
  struct FrameInFlight
  {
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformMemory = VK_NULL_HANDLE;
    void* m_pUniformMapped = nullptr;
  };

  nsResult CreateSwapChainResources();
  void DestroySwapChainResources();
  nsResult CreateFrameResources();
  void DestroyFrameResources();
  nsResult EnsureSwapChain();
  nsResult RecreateSwapChain();
  nsResult CreateDescriptorResources();
  void DestroyDescriptorResources();
  nsResult CreateGeometryBuffers();
  void DestroyGeometryBuffers();
  nsResult CreateShaderModules();
  void DestroyShaderModules();
  nsResult CreateGraphicsPipeline();
  void DestroyGraphicsPipeline();
  nsResult CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory);
  nsUInt32 FindMemoryType(nsUInt32 typeFilter, VkMemoryPropertyFlags properties) const;

  nsUniquePtr<nsVkInstance> m_pInstance;
  nsUniquePtr<nsVkDevice> m_pDevice;
  nsUniquePtr<nsVkSwapChain> m_pSwapChain;
  nsUniquePtr<nsVkCommandContext> m_pCommandContext;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  nsDynamicArray<VkFramebuffer> m_framebuffers;
  nsDynamicArray<FrameInFlight> m_framesInFlight;
  nsDynamicArray<VkFence> m_imagesInFlight;
  nsVkSwapChainCreateInfo m_swapChainCreateInfo;
  VkExtent2D m_currentExtent = {0, 0};
  VkExtent2D m_desiredExtent = {0, 0};
  nsUInt32 m_uiCurrentFrame = 0;
  nsUInt32 m_uiMaxFramesInFlight = 2;
  bool m_bResizePending = false;
  nsMat4 m_viewProjection = nsMat4::MakeIdentity();
  nsDynamicArray<nsVulkanInstanceData> m_sceneInstances;
  bool m_bSceneDirty = false;
  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
  VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
  nsUInt32 m_uiIndexCount = 0;
};
