#pragma once

#include <VulkanRenderer/Core/VkTypes.h>

struct NS_VULKANRENDERER_DLL nsVkInstanceCreateInfo
{
  nsString m_sApplicationName = "JDebug";
  nsUInt32 m_uiApplicationVersion = VK_MAKE_VERSION(0, 1, 0);
  nsDynamicArray<const char*> m_Extensions;
  bool m_bEnableValidation = true;
};

class NS_VULKANRENDERER_DLL nsVkInstance
{
public:
  nsVkInstance();
  ~nsVkInstance();

  nsResult Initialize(const nsVkInstanceCreateInfo& createInfo);
  void Deinitialize();

  VkInstance GetInstance() const { return m_instance; }

private:
  VkInstance m_instance = VK_NULL_HANDLE;
};
