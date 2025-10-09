#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/VulkanRendererModule.h>
#include <VulkanRenderer/DxcSupport.h>

#include <VulkanRenderer/Core/VkCommandContext.h>
#include <VulkanRenderer/Core/VkDevice.h>
#include <VulkanRenderer/Core/VkInstance.h>
#include <VulkanRenderer/Core/VkSwapChain.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Types/ScopeExit.h>

#include <array>

#include <dxcapi.h>

#include <limits>

#include <volk/volk.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <vulkan/vulkan_win32.h>
#endif

namespace
{
  struct alignas(16) SceneViewUniform
  {
    nsMat4 m_ViewProjection = nsMat4::MakeIdentity();
  };

  struct alignas(16) PushConstantData
  {
    nsMat4 m_Model = nsMat4::MakeIdentity();
    nsColor m_Color = nsColor::White;
  };

  static_assert(sizeof(PushConstantData) <= 128, "Push constant data exceeds Vulkan limit");

  constexpr const char* s_szVertexShaderPath = ":base/Shaders/VulkanRenderer/PvdSceneVS.hlsl";
  constexpr const char* s_szFragmentShaderPath = ":base/Shaders/VulkanRenderer/PvdScenePS.hlsl";

  nsResult LoadShaderSource(const char* szPath, nsDynamicArray<char>& outBuffer)
  {
    nsFileReader file;
    nsHybridArray<nsString, 8> attemptedPaths;
    nsString openedPath;

    auto TryOpen = [&](nsStringView path) -> bool {
      if (path.IsEmpty())
        return false;

      attemptedPaths.PushBack(path);
      if (file.Open(path).Succeeded())
      {
        openedPath = path;
        return true;
      }
      return false;
    };

    if (!TryOpen(szPath))
    {
      nsStringBuilder relativePath;

      if (nsStringUtils::IsEqualN(szPath, ":base/", 6))
      {
        relativePath = "Data/Base/";
        relativePath.Append(szPath + 6);
      }
      else if (szPath[0] == ':' && szPath[1] != '\0')
      {
        const char* szSlash = nsStringUtils::FindSubString(szPath, "/");
        if (szSlash != nullptr && *(szSlash + 1) != '\0')
        {
          relativePath = szSlash + 1;
        }
      }
      else
      {
        relativePath = szPath;
      }

      if (!relativePath.IsEmpty())
      {
        TryOpen(relativePath.GetView());

        nsHybridArray<nsString, 4> searchRoots;
        nsStringBuilder root = nsOSFile::GetApplicationDirectory();
        root.MakeCleanPath();

        for (nsUInt32 i = 0; i < 4 && !root.IsEmpty(); ++i)
        {
          searchRoots.PushBack(root.GetView());

          const nsUInt32 uiLengthBefore = root.GetCharacterCount();
          root.PathParentDirectory();
          root.MakeCleanPath();

          if (root.IsEmpty() || root.GetCharacterCount() >= uiLengthBefore)
            break;
        }

        for (const nsString& rootPath : searchRoots)
        {
          nsStringBuilder candidate = rootPath;
          candidate.AppendPath(relativePath);
          candidate.MakeCleanPath();

          if (TryOpen(candidate.GetView()))
            break;
        }
      }
    }

    if (openedPath.IsEmpty())
    {
      nsStringBuilder attemptsList;
      for (nsUInt32 i = 0; i < attemptedPaths.GetCount(); ++i)
      {
        if (i > 0)
          attemptsList.Append(", ");
        attemptsList.Append(attemptedPaths[i]);
      }

      nsLog::Error("Failed to open shader file '{0}'. Tried: {1}", szPath, attemptsList);
      return NS_FAILURE;
    }

    const nsUInt64 uiFileSize = file.GetFileSize();
    outBuffer.SetCount(static_cast<nsUInt32>(uiFileSize + 1));

    const nsUInt64 uiBytesRead = file.ReadBytes(outBuffer.GetData(), uiFileSize);
    if (uiBytesRead != uiFileSize)
    {
      nsLog::Error("Failed to read shader file '{0}'", openedPath);
      return NS_FAILURE;
    }

    outBuffer[static_cast<nsUInt32>(uiFileSize)] = '\0';
    return NS_SUCCESS;
  }
}

nsVulkanRenderer::nsVulkanRenderer() = default;
nsVulkanRenderer::~nsVulkanRenderer()
{
  Deinitialize();
}

nsResult nsVulkanRenderer::Initialize(const nsVulkanRendererCreateInfo& createInfo)
{
  NS_LOG_BLOCK("VulkanRenderer::Initialize");

  m_pInstance = NS_DEFAULT_NEW(nsVkInstance);

  nsVkInstanceCreateInfo instanceInfo;
  instanceInfo.m_Extensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  instanceInfo.m_Extensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
  if (createInfo.m_bEnableValidation)
  {
    if (!instanceInfo.m_Extensions.Contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
      instanceInfo.m_Extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
  }
  instanceInfo.m_bEnableValidation = createInfo.m_bEnableValidation;

  if (m_pInstance->Initialize(instanceInfo).Failed())
  {
    nsLog::Error("Failed to initialize Vulkan instance");
    return NS_FAILURE;
  }

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
  surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceInfo.hinstance = GetModuleHandle(nullptr);
  surfaceInfo.hwnd = static_cast<HWND>(createInfo.m_pWindowHandle);
  if (vkCreateWin32SurfaceKHR(m_pInstance->GetInstance(), &surfaceInfo, nullptr, &m_surface) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Win32 surface");
    return NS_FAILURE;
  }
#else
  NS_ASSERT_NOT_IMPLEMENTED;
#endif

  m_pDevice = NS_DEFAULT_NEW(nsVkDevice);

  nsVkDeviceCreateInfo deviceInfo;
  deviceInfo.m_surface = m_surface;
  deviceInfo.m_bEnableValidation = createInfo.m_bEnableValidation;

  if (m_pDevice->Initialize(*m_pInstance, deviceInfo).Failed())
  {
    nsLog::Error("Failed to initialize Vulkan device");
    return NS_FAILURE;
  }

  m_pSwapChain = NS_DEFAULT_NEW(nsVkSwapChain);
  m_pCommandContext = NS_DEFAULT_NEW(nsVkCommandContext);
  if (m_pCommandContext->Initialize(*m_pDevice, m_pDevice->GetGraphicsQueueFamily()).Failed())
  {
    nsLog::Error("Failed to initialize Vulkan command context");
    return NS_FAILURE;
  }

  m_swapChainCreateInfo.m_surface = m_surface;
  m_swapChainCreateInfo.m_uiGraphicsQueueFamily = m_pDevice->GetGraphicsQueueFamily();
  m_swapChainCreateInfo.m_uiPresentQueueFamily = m_pDevice->GetPresentQueueFamily();
  m_desiredExtent = {nsMath::Max<nsUInt32>(1, createInfo.m_uiWidth), nsMath::Max<nsUInt32>(1, createInfo.m_uiHeight)};
  m_uiCurrentFrame = 0;
  m_bResizePending = true;

  if (CreateShaderModules().Failed())
  {
    nsLog::Error("Failed to create Vulkan shader modules");
    return NS_FAILURE;
  }

  if (CreateDescriptorResources().Failed())
  {
    nsLog::Error("Failed to create Vulkan descriptor resources");
    return NS_FAILURE;
  }

  if (EnsureSwapChain().Failed())
  {
    nsLog::Error("Failed to create Vulkan swap-chain resources");
    return NS_FAILURE;
  }

  if (CreateFrameResources().Failed())
  {
    nsLog::Error("Failed to create Vulkan frame resources");
    return NS_FAILURE;
  }

  if (CreateDescriptorResources().Failed())
  {
    nsLog::Error("Failed to create Vulkan descriptor resources");
    return NS_FAILURE;
  }

  if (CreateGeometryBuffers().Failed())
  {
    nsLog::Error("Failed to create Vulkan geometry buffers");
    return NS_FAILURE;
  }

  if (CreateGraphicsPipeline().Failed())
  {
    nsLog::Error("Failed to create Vulkan graphics pipeline");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

void nsVulkanRenderer::SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight)
{
  VkExtent2D newExtent = {uiWidth, uiHeight};
  if (newExtent.width == 0 || newExtent.height == 0)
  {
    m_desiredExtent = newExtent;
    m_bResizePending = true;
    return;
  }

  newExtent.width = nsMath::Max<nsUInt32>(1, newExtent.width);
  newExtent.height = nsMath::Max<nsUInt32>(1, newExtent.height);

  if (newExtent.width == m_desiredExtent.width && newExtent.height == m_desiredExtent.height)
    return;

  m_desiredExtent = newExtent;
  m_bResizePending = true;
}

nsResult nsVulkanRenderer::CreateSwapChainResources()
{
  NS_ASSERT_DEV(m_pSwapChain != nullptr, "Swap-chain object must exist before creation");
  NS_ASSERT_DEV(m_pDevice != nullptr, "Vulkan device must be valid before creating swap-chain resources");

  if (m_desiredExtent.width == 0 || m_desiredExtent.height == 0)
    return NS_FAILURE;

  m_swapChainCreateInfo.m_extent = m_desiredExtent;
  m_swapChainCreateInfo.m_uiGraphicsQueueFamily = m_pDevice->GetGraphicsQueueFamily();
  m_swapChainCreateInfo.m_uiPresentQueueFamily = m_pDevice->GetPresentQueueFamily();

  if (m_pSwapChain->Initialize(*m_pInstance, *m_pDevice, m_swapChainCreateInfo).Failed())
  {
    nsLog::Error("Failed to initialize Vulkan swapchain");
    return NS_FAILURE;
  }

  VkDevice deviceHandle = m_pDevice->GetDevice();

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = m_pSwapChain->GetImageFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependencies[2] = {};
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = 0;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].dstAccessMask = 0;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependencies;

  if (vkCreateRenderPass(deviceHandle, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan render pass");
    DestroySwapChainResources();
    return NS_FAILURE;
  }

  const nsDynamicArray<VkImageView>& imageViews = m_pSwapChain->GetImageViews();
  m_framebuffers.SetCount(imageViews.GetCount());

  for (nsUInt32 i = 0; i < imageViews.GetCount(); ++i)
  {
    VkImageView attachments[] = {imageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = m_pSwapChain->GetExtent().width;
    framebufferInfo.height = m_pSwapChain->GetExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(deviceHandle, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan framebuffer");
      DestroySwapChainResources();
      return NS_FAILURE;
    }
  }

  m_imagesInFlight.SetCount(imageViews.GetCount());
  for (nsUInt32 i = 0; i < m_imagesInFlight.GetCount(); ++i)
  {
    m_imagesInFlight[i] = VK_NULL_HANDLE;
  }

  m_currentExtent = m_pSwapChain->GetExtent();
  m_desiredExtent = m_currentExtent;

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroySwapChainResources()
{
  DestroyGraphicsPipeline();

  if (m_pDevice != nullptr)
  {
    VkDevice deviceHandle = m_pDevice->GetDevice();
    if (deviceHandle != VK_NULL_HANDLE)
    {
      for (VkFramebuffer framebuffer : m_framebuffers)
      {
        if (framebuffer != VK_NULL_HANDLE)
        {
          vkDestroyFramebuffer(deviceHandle, framebuffer, nullptr);
        }
      }

      if (m_renderPass != VK_NULL_HANDLE)
      {
        vkDestroyRenderPass(deviceHandle, m_renderPass, nullptr);
      }
    }
  }

  m_framebuffers.Clear();
  m_renderPass = VK_NULL_HANDLE;
  m_imagesInFlight.Clear();

  if (m_pSwapChain)
  {
    m_pSwapChain->Deinitialize(*m_pDevice);
  }

  m_currentExtent = {0, 0};
}

nsResult nsVulkanRenderer::EnsureSwapChain()
{
  if (m_pSwapChain == nullptr)
    return NS_FAILURE;

  if (!m_bResizePending && m_currentExtent.width != 0 && m_currentExtent.height != 0)
    return NS_SUCCESS;

  if (m_desiredExtent.width == 0 || m_desiredExtent.height == 0)
  {
    // Window minimized or zero-sized — skip swap-chain recreation for now.
    return NS_SUCCESS;
  }

  return RecreateSwapChain();
}

nsResult nsVulkanRenderer::RecreateSwapChain()
{
  if (m_pDevice == nullptr || m_pSwapChain == nullptr)
    return NS_FAILURE;

  VkDevice deviceHandle = m_pDevice->GetDevice();
  if (deviceHandle == VK_NULL_HANDLE)
    return NS_FAILURE;

  vkDeviceWaitIdle(deviceHandle);

  DestroySwapChainResources();

  m_swapChainCreateInfo.m_uiGraphicsQueueFamily = m_pDevice->GetGraphicsQueueFamily();
  m_swapChainCreateInfo.m_uiPresentQueueFamily = m_pDevice->GetPresentQueueFamily();

  nsResult result = CreateSwapChainResources();
  if (result.Failed())
    return result;

  if (CreateGraphicsPipeline().Failed())
    return NS_FAILURE;

  if (!m_framesInFlight.IsEmpty())
  {
    const nsUInt32 imageCount = m_pSwapChain->GetImageCount();
    m_imagesInFlight.SetCount(imageCount);
    for (nsUInt32 i = 0; i < imageCount; ++i)
    {
      m_imagesInFlight[i] = VK_NULL_HANDLE;
    }
  }

  m_uiCurrentFrame = 0;
  m_bResizePending = false;

  return NS_SUCCESS;
}

nsUInt32 nsVulkanRenderer::FindMemoryType(nsUInt32 typeFilter, VkMemoryPropertyFlags properties) const
{
  if (!m_pDevice)
    return 0;

  VkPhysicalDevice physicalDevice = m_pDevice->GetPhysicalDevice();
  if (physicalDevice == VK_NULL_HANDLE)
    return 0;

  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (nsUInt32 i = 0; i < memProperties.memoryTypeCount; ++i)
  {
    if ((typeFilter & (1u << i)) != 0 && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  NS_ASSERT_DEV(false, "Failed to find suitable Vulkan memory type");
  return 0;
}

nsResult nsVulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory)
{
  if (!m_pDevice)
    return NS_FAILURE;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return NS_FAILURE;

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan buffer");
    return NS_FAILURE;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
  {
    nsLog::Error("Failed to allocate Vulkan buffer memory");
    vkDestroyBuffer(device, outBuffer, nullptr);
    outBuffer = VK_NULL_HANDLE;
    return NS_FAILURE;
  }

  if (vkBindBufferMemory(device, outBuffer, outMemory, 0) != VK_SUCCESS)
  {
    nsLog::Error("Failed to bind Vulkan buffer memory");
    vkDestroyBuffer(device, outBuffer, nullptr);
    vkFreeMemory(device, outMemory, nullptr);
    outBuffer = VK_NULL_HANDLE;
    outMemory = VK_NULL_HANDLE;
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsVulkanRenderer::CreateDescriptorResources()
{
  if (!m_pDevice)
    return NS_FAILURE;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_descriptorSetLayout == VK_NULL_HANDLE)
  {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan descriptor set layout");
      return NS_FAILURE;
    }
  }

  if (m_descriptorPool == VK_NULL_HANDLE)
  {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = m_uiMaxFramesInFlight;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = m_uiMaxFramesInFlight;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan descriptor pool");
      return NS_FAILURE;
    }
  }

  if (m_framesInFlight.IsEmpty())
    return NS_SUCCESS;

  nsDynamicArray<VkDescriptorSetLayout> layouts;
  layouts.SetCount(m_framesInFlight.GetCount());
  for (nsUInt32 i = 0; i < layouts.GetCount(); ++i)
  {
    layouts[i] = m_descriptorSetLayout;
  }

  nsDynamicArray<VkDescriptorSet> descriptorSets;
  descriptorSets.SetCount(m_framesInFlight.GetCount());

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_descriptorPool;
  allocInfo.descriptorSetCount = static_cast<nsUInt32>(layouts.GetCount());
  allocInfo.pSetLayouts = layouts.GetData();

  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.GetData()) != VK_SUCCESS)
  {
    nsLog::Error("Failed to allocate Vulkan descriptor sets");
    return NS_FAILURE;
  }

  for (nsUInt32 i = 0; i < m_framesInFlight.GetCount(); ++i)
  {
    FrameInFlight& frame = m_framesInFlight[i];
    frame.m_descriptorSet = descriptorSets[i];

    if (frame.m_uniformBuffer == VK_NULL_HANDLE)
    {
      if (CreateBuffer(sizeof(SceneViewUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, frame.m_uniformBuffer, frame.m_uniformMemory).Failed())
      {
        nsLog::Error("Failed to create Vulkan uniform buffer");
        return NS_FAILURE;
      }

      if (vkMapMemory(device, frame.m_uniformMemory, 0, sizeof(SceneViewUniform), 0, &frame.m_pUniformMapped) != VK_SUCCESS)
      {
        nsLog::Error("Failed to map Vulkan uniform buffer");
        vkDestroyBuffer(device, frame.m_uniformBuffer, nullptr);
        frame.m_uniformBuffer = VK_NULL_HANDLE;
        vkFreeMemory(device, frame.m_uniformMemory, nullptr);
        frame.m_uniformMemory = VK_NULL_HANDLE;
        return NS_FAILURE;
      }
    }

    if (frame.m_pUniformMapped)
    {
      SceneViewUniform* pUniform = static_cast<SceneViewUniform*>(frame.m_pUniformMapped);
      *pUniform = SceneViewUniform{};
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = frame.m_uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(SceneViewUniform);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = frame.m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  }

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroyDescriptorResources()
{
  if (!m_pDevice)
    return;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return;

  for (FrameInFlight& frame : m_framesInFlight)
  {
    if (frame.m_pUniformMapped)
    {
      vkUnmapMemory(device, frame.m_uniformMemory);
      frame.m_pUniformMapped = nullptr;
    }

    if (frame.m_uniformBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, frame.m_uniformBuffer, nullptr);
      frame.m_uniformBuffer = VK_NULL_HANDLE;
    }

    if (frame.m_uniformMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, frame.m_uniformMemory, nullptr);
      frame.m_uniformMemory = VK_NULL_HANDLE;
    }

    frame.m_descriptorSet = VK_NULL_HANDLE;
  }

  if (m_descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
  }
}

nsResult nsVulkanRenderer::CreateGeometryBuffers()
{
  if (!m_pDevice)
    return NS_FAILURE;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_vertexBuffer != VK_NULL_HANDLE && m_indexBuffer != VK_NULL_HANDLE)
    return NS_SUCCESS;

  static constexpr std::array<float, 3 * 8> s_cubeVertices = {
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
  };

  static constexpr std::array<nsUInt16, 36> s_cubeIndices = {
    0, 1, 2, 2, 3, 0, // back
    4, 5, 6, 6, 7, 4, // front
    4, 5, 1, 1, 0, 4, // bottom
    7, 6, 2, 2, 3, 7, // top
    5, 6, 2, 2, 1, 5, // right
    4, 7, 3, 3, 0, 4  // left
  };

  VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(s_cubeVertices.size() * sizeof(float));
  VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(s_cubeIndices.size() * sizeof(nsUInt16));

  if (CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer, m_vertexBufferMemory).Failed())
  {
    nsLog::Error("Failed to create Vulkan vertex buffer");
    return NS_FAILURE;
  }

  void* pVertexData = nullptr;
  if (vkMapMemory(device, m_vertexBufferMemory, 0, vertexBufferSize, 0, &pVertexData) != VK_SUCCESS)
  {
    nsLog::Error("Failed to map Vulkan vertex buffer");
    return NS_FAILURE;
  }
  nsMemoryUtils::Copy(static_cast<float*>(pVertexData), s_cubeVertices.data(), s_cubeVertices.size());
  vkUnmapMemory(device, m_vertexBufferMemory);

  if (CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffer, m_indexBufferMemory).Failed())
  {
    nsLog::Error("Failed to create Vulkan index buffer");
    return NS_FAILURE;
  }

  void* pIndexData = nullptr;
  if (vkMapMemory(device, m_indexBufferMemory, 0, indexBufferSize, 0, &pIndexData) != VK_SUCCESS)
  {
    nsLog::Error("Failed to map Vulkan index buffer");
    return NS_FAILURE;
  }
  nsMemoryUtils::Copy(static_cast<nsUInt16*>(pIndexData), s_cubeIndices.data(), s_cubeIndices.size());
  vkUnmapMemory(device, m_indexBufferMemory);

  m_uiIndexCount = static_cast<nsUInt32>(s_cubeIndices.size());

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroyGeometryBuffers()
{
  if (!m_pDevice)
    return;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return;

  if (m_vertexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
  }

  if (m_vertexBufferMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, m_vertexBufferMemory, nullptr);
    m_vertexBufferMemory = VK_NULL_HANDLE;
  }

  if (m_indexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
  }

  if (m_indexBufferMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, m_indexBufferMemory, nullptr);
    m_indexBufferMemory = VK_NULL_HANDLE;
  }

  m_uiIndexCount = 0;
}

nsResult nsVulkanRenderer::CreateShaderModules()
{
  if (!m_pDevice)
    return NS_FAILURE;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_vertexShaderModule != VK_NULL_HANDLE && m_fragmentShaderModule != VK_NULL_HANDLE)
    return NS_SUCCESS;

  nsDynamicArray<char> vertexSource;
  NS_SUCCEED_OR_RETURN(LoadShaderSource(s_szVertexShaderPath, vertexSource));

  nsDynamicArray<char> fragmentSource;
  NS_SUCCEED_OR_RETURN(LoadShaderSource(s_szFragmentShaderPath, fragmentSource));

  auto MakeSourceView = [](const nsDynamicArray<char>& buffer) {
    const nsUInt32 count = buffer.GetCount();
    if (count == 0)
      return nsArrayPtr<const char>();
    return nsArrayPtr<const char>(buffer.GetData(), count - 1);
  };

  nsArrayPtr<const char> vertexSourceView = MakeSourceView(vertexSource);
  nsArrayPtr<const char> fragmentSourceView = MakeSourceView(fragmentSource);

  nsVulkanDxc::CreateInstanceProc pfnCreateInstance = nullptr;
  if (nsVulkanDxc::ResolveCreateInstance(pfnCreateInstance).Failed() || pfnCreateInstance == nullptr)
  {
    nsLog::Error("DXC runtime is not available on this platform");
    return NS_FAILURE;
  }

  auto hresultHex = [](HRESULT value) {
    return nsArgU(static_cast<nsUInt64>(static_cast<nsUInt32>(value)), 8, true, 16, true);
  };

  IDxcUtils* pUtils = nullptr;
  HRESULT hr = pfnCreateInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), reinterpret_cast<void**>(&pUtils));
  if (FAILED(hr) || pUtils == nullptr)
  {
    nsLog::Error("Failed to create DxcUtils instance (HRESULT: 0x{0})", hresultHex(hr));
    return NS_FAILURE;
  }
  NS_SCOPE_EXIT(if (pUtils) { pUtils->Release(); });

  IDxcCompiler3* pCompiler = nullptr;
  hr = pfnCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(&pCompiler));
  if (FAILED(hr) || pCompiler == nullptr)
  {
    nsLog::Error("Failed to create DxcCompiler instance (HRESULT: 0x{0})", hresultHex(hr));
    return NS_FAILURE;
  }
  NS_SCOPE_EXIT(if (pCompiler) { pCompiler->Release(); });

  IDxcIncludeHandler* pIncludeHandler = nullptr;
  hr = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
  if (FAILED(hr) || pIncludeHandler == nullptr)
  {
    nsLog::Error("Failed to create Dxc include handler (HRESULT: 0x{0})", hresultHex(hr));
    return NS_FAILURE;
  }
  NS_SCOPE_EXIT(if (pIncludeHandler) { pIncludeHandler->Release(); });

  auto CompileShader = [&](nsArrayPtr<const char> source, const wchar_t* entryPoint, const wchar_t* targetProfile, const char* szDebugName, IDxcBlob*& outBlob) -> nsResult {
    if (source.IsEmpty())
    {
      nsLog::Error("Shader source '{0}' is empty", szDebugName);
      return NS_FAILURE;
    }

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = source.GetPtr();
    sourceBuffer.Size = static_cast<SIZE_T>(source.GetCount());
    sourceBuffer.Encoding = DXC_CP_UTF8;

    const wchar_t* arguments[] = {
      L"-E", entryPoint,
      L"-T", targetProfile,
      L"-spirv",
      L"-fspv-target-env=vulkan1.2",
      L"-fvk-use-dx-layout",
      L"-O0"
    };

    IDxcResult* pResult = nullptr;
    HRESULT compileHr = pCompiler->Compile(&sourceBuffer, arguments, NS_ARRAY_SIZE(arguments), pIncludeHandler, __uuidof(IDxcResult), reinterpret_cast<void**>(&pResult));
    if (FAILED(compileHr) || pResult == nullptr)
    {
      nsLog::Error("Failed to compile shader '{0}' with DXC (HRESULT: 0x{1})", szDebugName, hresultHex(compileHr));
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pResult) { pResult->Release(); });

    HRESULT status = S_OK;
    if (FAILED(pResult->GetStatus(&status)) || FAILED(status))
    {
      IDxcBlobUtf8* pErrors = nullptr;
      if (SUCCEEDED(pResult->GetOutput(DXC_OUT_ERRORS, __uuidof(IDxcBlobUtf8), reinterpret_cast<void**>(&pErrors), nullptr)) && pErrors && pErrors->GetStringLength() > 0)
      {
        nsLog::Error("DXC compilation error: {0}", pErrors->GetStringPointer());
      }
      if (pErrors)
      {
        pErrors->Release();
      }
      return NS_FAILURE;
    }

    if (FAILED(pResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), reinterpret_cast<void**>(&outBlob), nullptr)) || outBlob == nullptr)
    {
      nsLog::Error("Failed to retrieve compiled shader blob");
      return NS_FAILURE;
    }

    return NS_SUCCESS;
  };

  IDxcBlob* pVertexBlob = nullptr;
  NS_SCOPE_EXIT(if (pVertexBlob) { pVertexBlob->Release(); });
  NS_SUCCEED_OR_RETURN(CompileShader(vertexSourceView, L"mainVS", L"vs_6_0", s_szVertexShaderPath, pVertexBlob));

  IDxcBlob* pFragmentBlob = nullptr;
  NS_SCOPE_EXIT(if (pFragmentBlob) { pFragmentBlob->Release(); });
  NS_SUCCEED_OR_RETURN(CompileShader(fragmentSourceView, L"mainPS", L"ps_6_0", s_szFragmentShaderPath, pFragmentBlob));

  VkShaderModuleCreateInfo vertexModuleInfo = {};
  vertexModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertexModuleInfo.codeSize = pVertexBlob->GetBufferSize();
  vertexModuleInfo.pCode = reinterpret_cast<const nsUInt32*>(pVertexBlob->GetBufferPointer());

  if (vkCreateShaderModule(device, &vertexModuleInfo, nullptr, &m_vertexShaderModule) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan vertex shader module");
    return NS_FAILURE;
  }

  VkShaderModuleCreateInfo fragmentModuleInfo = {};
  fragmentModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragmentModuleInfo.codeSize = pFragmentBlob->GetBufferSize();
  fragmentModuleInfo.pCode = reinterpret_cast<const nsUInt32*>(pFragmentBlob->GetBufferPointer());

  if (vkCreateShaderModule(device, &fragmentModuleInfo, nullptr, &m_fragmentShaderModule) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan fragment shader module");
    vkDestroyShaderModule(device, m_vertexShaderModule, nullptr);
    m_vertexShaderModule = VK_NULL_HANDLE;
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroyShaderModules()
{
  if (!m_pDevice)
    return;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return;

  if (m_vertexShaderModule != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(device, m_vertexShaderModule, nullptr);
    m_vertexShaderModule = VK_NULL_HANDLE;
  }

  if (m_fragmentShaderModule != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(device, m_fragmentShaderModule, nullptr);
    m_fragmentShaderModule = VK_NULL_HANDLE;
  }
}

nsResult nsVulkanRenderer::CreateGraphicsPipeline()
{
  if (!m_pDevice)
    return NS_FAILURE;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_renderPass == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_vertexShaderModule == VK_NULL_HANDLE || m_fragmentShaderModule == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_descriptorSetLayout == VK_NULL_HANDLE)
    return NS_FAILURE;

  if (m_graphicsPipeline != VK_NULL_HANDLE)
    return NS_SUCCESS;

  VkPipelineShaderStageCreateInfo shaderStages[2] = {};
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = m_vertexShaderModule;
  shaderStages[0].pName = "mainVS";

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = m_fragmentShaderModule;
  shaderStages[1].pName = "mainPS";

  VkVertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(float) * 3;
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescription = {};
  attributeDescription.location = 0;
  attributeDescription.binding = 0;
  attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription.offset = 0;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 1;
  vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = 1.0f;
  viewport.height = 1.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = {1, 1};

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.sampleShadingEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = NS_ARRAY_SIZE(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

  if (m_pipelineLayout == VK_NULL_HANDLE)
  {
    VkPushConstantRange pushConstant = {};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan pipeline layout");
      return NS_FAILURE;
    }
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = NS_ARRAY_SIZE(shaderStages);
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.renderPass = m_renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan graphics pipeline");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroyGraphicsPipeline()
{
  if (!m_pDevice)
    return;

  VkDevice device = m_pDevice->GetDevice();
  if (device == VK_NULL_HANDLE)
    return;

  if (m_graphicsPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, m_graphicsPipeline, nullptr);
    m_graphicsPipeline = VK_NULL_HANDLE;
  }

  if (m_pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }
}

nsResult nsVulkanRenderer::CreateFrameResources()
{
  NS_ASSERT_DEV(m_pDevice != nullptr, "Vulkan device must be valid before creating frame resources");
  NS_ASSERT_DEV(m_pCommandContext != nullptr, "Vulkan command context must be valid before creating frame resources");

  VkDevice deviceHandle = m_pDevice->GetDevice();
  if (deviceHandle == VK_NULL_HANDLE)
    return NS_FAILURE;

  m_framesInFlight.Clear();
  m_framesInFlight.SetCount(m_uiMaxFramesInFlight);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_pCommandContext->GetCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = m_uiMaxFramesInFlight;

  nsHybridArray<VkCommandBuffer, 4> commandBuffers;
  commandBuffers.SetCount(m_uiMaxFramesInFlight);

  if (vkAllocateCommandBuffers(deviceHandle, &allocInfo, commandBuffers.GetData()) != VK_SUCCESS)
  {
    nsLog::Error("Failed to allocate Vulkan command buffers");
    m_framesInFlight.Clear();
    return NS_FAILURE;
  }

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (nsUInt32 i = 0; i < m_uiMaxFramesInFlight; ++i)
  {
    FrameInFlight& frame = m_framesInFlight[i];
    frame.m_commandBuffer = commandBuffers[i];

    if (vkCreateSemaphore(deviceHandle, &semaphoreInfo, nullptr, &frame.m_imageAvailableSemaphore) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan semaphore (image available)");
      DestroyFrameResources();
      return NS_FAILURE;
    }

    if (vkCreateSemaphore(deviceHandle, &semaphoreInfo, nullptr, &frame.m_renderFinishedSemaphore) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan semaphore (render finished)");
      DestroyFrameResources();
      return NS_FAILURE;
    }

    if (vkCreateFence(deviceHandle, &fenceInfo, nullptr, &frame.m_inFlightFence) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan fence");
      DestroyFrameResources();
      return NS_FAILURE;
    }
  }

  if (!m_pSwapChain)
    return NS_SUCCESS;

  const nsUInt32 imageCount = m_pSwapChain->GetImageCount();
  if (m_imagesInFlight.GetCount() != imageCount)
  {
    m_imagesInFlight.SetCount(imageCount);
    for (nsUInt32 i = 0; i < imageCount; ++i)
    {
      m_imagesInFlight[i] = VK_NULL_HANDLE;
    }
  }

  m_uiCurrentFrame = 0;

  return NS_SUCCESS;
}

void nsVulkanRenderer::DestroyFrameResources()
{
  if (m_pDevice != nullptr)
  {
    VkDevice deviceHandle = m_pDevice->GetDevice();
    if (deviceHandle != VK_NULL_HANDLE)
    {
      vkDeviceWaitIdle(deviceHandle);

      DestroyDescriptorResources();

      if (m_pCommandContext != nullptr)
      {
        VkCommandPool pool = m_pCommandContext->GetCommandPool();
        if (pool != VK_NULL_HANDLE)
        {
          for (FrameInFlight& frame : m_framesInFlight)
          {
            if (frame.m_commandBuffer != VK_NULL_HANDLE)
            {
              vkFreeCommandBuffers(deviceHandle, pool, 1, &frame.m_commandBuffer);
              frame.m_commandBuffer = VK_NULL_HANDLE;
            }
          }
        }
      }

      for (FrameInFlight& frame : m_framesInFlight)
      {
        if (frame.m_imageAvailableSemaphore != VK_NULL_HANDLE)
        {
          vkDestroySemaphore(deviceHandle, frame.m_imageAvailableSemaphore, nullptr);
          frame.m_imageAvailableSemaphore = VK_NULL_HANDLE;
        }

        if (frame.m_renderFinishedSemaphore != VK_NULL_HANDLE)
        {
          vkDestroySemaphore(deviceHandle, frame.m_renderFinishedSemaphore, nullptr);
          frame.m_renderFinishedSemaphore = VK_NULL_HANDLE;
        }

        if (frame.m_inFlightFence != VK_NULL_HANDLE)
        {
          vkDestroyFence(deviceHandle, frame.m_inFlightFence, nullptr);
          frame.m_inFlightFence = VK_NULL_HANDLE;
        }
      }
    }
  }

  m_framesInFlight.Clear();
  m_imagesInFlight.Clear();
  m_uiCurrentFrame = 0;
}

void nsVulkanRenderer::UpdateScene(const nsMat4& mViewProjection, nsArrayPtr<const nsVulkanInstanceData> instances)
{
  m_viewProjection = mViewProjection;

  m_sceneInstances.SetCount(instances.GetCount());
  if (!instances.IsEmpty())
  {
    nsMemoryUtils::Copy(m_sceneInstances.GetData(), instances.GetPtr(), instances.GetCount());
  }

  m_bSceneDirty = true;
}

nsResult nsVulkanRenderer::RenderFrame()
{
  if (m_pDevice == nullptr || m_pSwapChain == nullptr || m_pCommandContext == nullptr)
  {
    nsLog::Error("Vulkan renderer is not initialized");
    return NS_FAILURE;
  }

  if (m_desiredExtent.width == 0 || m_desiredExtent.height == 0)
  {
    // Minimized window — skip rendering for now.
    return NS_SUCCESS;
  }

  NS_SUCCEED_OR_RETURN(EnsureSwapChain());

  if (m_framebuffers.IsEmpty())
    return NS_FAILURE;

  FrameInFlight& frame = m_framesInFlight[m_uiCurrentFrame];
  if (frame.m_commandBuffer == VK_NULL_HANDLE || frame.m_inFlightFence == VK_NULL_HANDLE)
  {
    nsLog::Error("Vulkan frame resources are not initialized");
    return NS_FAILURE;
  }

  VkDevice deviceHandle = m_pDevice->GetDevice();

  VkResult waitResult = vkWaitForFences(deviceHandle, 1, &frame.m_inFlightFence, VK_TRUE, std::numeric_limits<nsUInt64>::max());
  if (waitResult != VK_SUCCESS)
  {
    nsLog::Error("Failed to wait for Vulkan fence");
    return NS_FAILURE;
  }

  nsUInt32 imageIndex = 0;
  VkResult acquireResult = vkAcquireNextImageKHR(deviceHandle, m_pSwapChain->GetSwapChain(), std::numeric_limits<nsUInt64>::max(), frame.m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m_bResizePending = true;
    NS_SUCCEED_OR_RETURN(EnsureSwapChain());
    return NS_SUCCESS;
  }

  if (acquireResult == VK_SUBOPTIMAL_KHR)
  {
    m_bResizePending = true;
  }
  else if (acquireResult != VK_SUCCESS)
  {
    nsLog::Error("Failed to acquire Vulkan swap-chain image");
    return NS_FAILURE;
  }

  if (vkResetFences(deviceHandle, 1, &frame.m_inFlightFence) != VK_SUCCESS)
  {
    nsLog::Error("Failed to reset Vulkan fence");
    return NS_FAILURE;
  }

  if (imageIndex >= m_framebuffers.GetCount())
  {
    nsLog::Error("Acquired swap-chain image index is out of range");
    return NS_FAILURE;
  }

  if (m_imagesInFlight.GetCount() > imageIndex)
  {
    VkFence& imageFence = m_imagesInFlight[imageIndex];
    if (imageFence != VK_NULL_HANDLE && imageFence != frame.m_inFlightFence)
    {
      vkWaitForFences(deviceHandle, 1, &imageFence, VK_TRUE, std::numeric_limits<nsUInt64>::max());
    }
    m_imagesInFlight[imageIndex] = frame.m_inFlightFence;
  }

  if (vkResetCommandBuffer(frame.m_commandBuffer, 0) != VK_SUCCESS)
  {
    nsLog::Error("Failed to reset Vulkan command buffer");
    return NS_FAILURE;
  }

  if (frame.m_pUniformMapped)
  {
    SceneViewUniform* pUniform = static_cast<SceneViewUniform*>(frame.m_pUniformMapped);
    pUniform->m_ViewProjection = m_viewProjection;
  }

  if (m_graphicsPipeline == VK_NULL_HANDLE || m_pipelineLayout == VK_NULL_HANDLE)
  {
    nsLog::Error("Vulkan graphics pipeline is not initialized");
    return NS_FAILURE;
  }

  if (frame.m_descriptorSet == VK_NULL_HANDLE)
  {
    nsLog::Error("Vulkan descriptor set is not initialized for current frame");
    return NS_FAILURE;
  }

  if (m_vertexBuffer == VK_NULL_HANDLE || m_indexBuffer == VK_NULL_HANDLE || m_uiIndexCount == 0)
  {
    nsLog::Error("Vulkan geometry buffers are not initialized");
    return NS_FAILURE;
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(frame.m_commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    nsLog::Error("Failed to begin Vulkan command buffer");
    return NS_FAILURE;
  }

  VkClearValue clearColor = {};
  clearColor.color = {{0.02f, 0.05f, 0.09f, 1.0f}};

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = m_framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = m_currentExtent;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(frame.m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(m_currentExtent.width);
  viewport.height = static_cast<float>(m_currentExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(frame.m_commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = m_currentExtent;
  vkCmdSetScissor(frame.m_commandBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(frame.m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
  vkCmdBindDescriptorSets(frame.m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &frame.m_descriptorSet, 0, nullptr);

  VkBuffer vertexBuffers[] = {m_vertexBuffer};
  VkDeviceSize vertexOffsets[] = {0};
  vkCmdBindVertexBuffers(frame.m_commandBuffer, 0, 1, vertexBuffers, vertexOffsets);
  vkCmdBindIndexBuffer(frame.m_commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  for (const nsVulkanInstanceData& instance : m_sceneInstances)
  {
    if (instance.m_bSleeping)
      continue;

    PushConstantData pushConstants;
    pushConstants.m_Model = instance.m_ModelMatrix;
    pushConstants.m_Color = instance.m_Color;

    vkCmdPushConstants(frame.m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &pushConstants);
    vkCmdDrawIndexed(frame.m_commandBuffer, m_uiIndexCount, 1, 0, 0, 0);
  }

  m_bSceneDirty = false;

  vkCmdEndRenderPass(frame.m_commandBuffer);

  if (vkEndCommandBuffer(frame.m_commandBuffer) != VK_SUCCESS)
  {
    nsLog::Error("Failed to end Vulkan command buffer");
    return NS_FAILURE;
  }

  VkSemaphore waitSemaphores[] = {frame.m_imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {frame.m_renderFinishedSemaphore};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &frame.m_commandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, frame.m_inFlightFence) != VK_SUCCESS)
  {
    nsLog::Error("Failed to submit Vulkan command buffer");
    return NS_FAILURE;
  }

  VkSwapchainKHR swapChains[] = {m_pSwapChain->GetSwapChain()};

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  VkResult presentResult = vkQueuePresentKHR(m_pDevice->GetPresentQueue(), &presentInfo);
  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
  {
    m_bResizePending = true;
    NS_SUCCEED_OR_RETURN(EnsureSwapChain());
  }
  else if (presentResult != VK_SUCCESS)
  {
    nsLog::Error("Failed to present Vulkan swap-chain image");
    return NS_FAILURE;
  }

  m_uiCurrentFrame = (m_uiCurrentFrame + 1) % m_framesInFlight.GetCount();

  return NS_SUCCESS;
}

void nsVulkanRenderer::Deinitialize()
{
  DestroyFrameResources();
  DestroySwapChainResources();
  DestroyGeometryBuffers();
  DestroyShaderModules();

  if (m_pDevice && m_descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorSetLayout, nullptr);
    m_descriptorSetLayout = VK_NULL_HANDLE;
  }

  if (m_pCommandContext)
  {
    if (m_pDevice)
    {
      m_pCommandContext->Deinitialize(*m_pDevice);
    }
    m_pCommandContext = nullptr;
  }

  m_pSwapChain = nullptr;

  if (m_pDevice)
  {
    m_pDevice->Deinitialize();
    m_pDevice = nullptr;
  }

  if (m_surface != VK_NULL_HANDLE && m_pInstance)
  {
    vkDestroySurfaceKHR(m_pInstance->GetInstance(), m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }

  if (m_pInstance)
  {
    m_pInstance->Deinitialize();
    m_pInstance = nullptr;
  }
}

NS_STATICLINK_FILE(VulkanRenderer, VulkanRenderer_VulkanRendererModule);
