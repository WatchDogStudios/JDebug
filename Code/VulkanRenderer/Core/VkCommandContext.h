#pragma once

#include <VulkanRenderer/Core/VkTypes.h>

class nsVkDevice;

class NS_VULKANRENDERER_DLL nsVkCommandContext
{
public:
  nsVkCommandContext();
  ~nsVkCommandContext();

  nsResult Initialize(const nsVkDevice& device, nsUInt32 queueFamily);
  void Deinitialize(const nsVkDevice& device);

  VkCommandPool GetCommandPool() const { return m_commandPool; }

private:
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
};
