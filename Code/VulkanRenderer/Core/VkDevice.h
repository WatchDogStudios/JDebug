#pragma once

#include <VulkanRenderer/Core/VkTypes.h>
#include <Foundation/Types/Types.h>

class nsVkInstance;

struct NS_VULKANRENDERER_DLL nsVkDeviceCreateInfo
{
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  bool m_bEnableValidation = true;
};

class NS_VULKANRENDERER_DLL nsVkDevice
{
public:
  nsVkDevice();
  ~nsVkDevice();

  nsResult Initialize(const nsVkInstance& instance, const nsVkDeviceCreateInfo& createInfo);
  void Deinitialize();

  VkDevice GetDevice() const { return m_device; }
  VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
  VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
  nsUInt32 GetGraphicsQueueFamily() const { return m_uiGraphicsQueueFamily; }
  VkQueue GetPresentQueue() const { return m_presentQueue; }
  nsUInt32 GetPresentQueueFamily() const { return m_uiPresentQueueFamily; }

private:
  VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
  nsResult CreateLogicalDevice(VkSurfaceKHR surface, bool bEnableValidation);

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  nsUInt32 m_uiGraphicsQueueFamily = nsInvalidIndex;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  nsUInt32 m_uiPresentQueueFamily = nsInvalidIndex;
};
