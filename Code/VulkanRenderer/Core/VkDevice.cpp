#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/Core/VkDevice.h>
#include <VulkanRenderer/Core/VkInstance.h>

#include <volk/volk.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>

namespace
{
  struct QueueFamilyIndices
  {
  nsUInt32 m_uiGraphicsFamily = nsInvalidIndex;
  nsUInt32 m_uiPresentFamily = nsInvalidIndex;

  bool IsComplete() const { return m_uiGraphicsFamily != nsInvalidIndex && m_uiPresentFamily != nsInvalidIndex; }
  };

  QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
  {
    QueueFamilyIndices indices;

    nsUInt32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    nsDynamicArray<VkQueueFamilyProperties> queueFamilies;
    queueFamilies.SetCount(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.GetData());

    for (nsUInt32 i = 0; i < queueFamilyCount; ++i)
    {
      const auto& props = queueFamilies[i];
      if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.m_uiGraphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport)
      {
        indices.m_uiPresentFamily = i;
      }

      if (indices.IsComplete())
        break;
    }

    return indices;
  }

  nsDynamicArray<const char*> GetRequiredDeviceExtensions()
  {
    nsDynamicArray<const char*> extensions;
    extensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    extensions.PushBack(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    return extensions;
  }
}

nsVkDevice::nsVkDevice() = default;
nsVkDevice::~nsVkDevice()
{
  Deinitialize();
}

nsResult nsVkDevice::Initialize(const nsVkInstance& instance, const nsVkDeviceCreateInfo& createInfo)
{
  NS_ASSERT_DEV(m_device == VK_NULL_HANDLE, "Device already created");

  m_physicalDevice = PickPhysicalDevice(instance.GetInstance(), createInfo.m_surface);
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    nsLog::Error("Failed to find suitable Vulkan physical device");
    return NS_FAILURE;
  }

  return CreateLogicalDevice(createInfo.m_surface, createInfo.m_bEnableValidation);
}

void nsVkDevice::Deinitialize()
{
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }
  m_physicalDevice = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_presentQueue = VK_NULL_HANDLE;
  m_uiGraphicsQueueFamily = nsInvalidIndex;
  m_uiPresentQueueFamily = nsInvalidIndex;
}

VkPhysicalDevice nsVkDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
  nsUInt32 deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
  {
    nsLog::Error("No Vulkan physical devices available");
    return VK_NULL_HANDLE;
  }

  nsDynamicArray<VkPhysicalDevice> devices;
  devices.SetCount(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.GetData());

  for (VkPhysicalDevice device : devices)
  {
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);
    if (!indices.IsComplete())
      continue;

    nsDynamicArray<const char*> extensions = GetRequiredDeviceExtensions();

    nsUInt32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    nsDynamicArray<VkExtensionProperties> availableExtensions;
    availableExtensions.SetCount(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.GetData());

    bool bExtensionsSupported = true;
    for (const char* ext : extensions)
    {
      bool bFound = false;
      for (const auto& available : availableExtensions)
      {
        if (nsStringUtils::IsEqual(ext, available.extensionName))
        {
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        bExtensionsSupported = false;
        break;
      }
    }

    if (!bExtensionsSupported)
      continue;

    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(device, &features);

    if (!features.samplerAnisotropy)
      continue;

    QueueFamilyIndices finalIndices = FindQueueFamilies(device, surface);
    m_uiGraphicsQueueFamily = finalIndices.m_uiGraphicsFamily;
    m_uiPresentQueueFamily = finalIndices.m_uiPresentFamily;
    return device;
  }

  return VK_NULL_HANDLE;
}

nsResult nsVkDevice::CreateLogicalDevice(VkSurfaceKHR surface, bool bEnableValidation)
{
  QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, surface);

  nsHybridArray<VkDeviceQueueCreateInfo, 2> queueInfos;
  nsHybridArray<nsUInt32, 2> uniqueQueueFamilies;
  uniqueQueueFamilies.PushBack(indices.m_uiGraphicsFamily);
  if (indices.m_uiPresentFamily != indices.m_uiGraphicsFamily)
    uniqueQueueFamilies.PushBack(indices.m_uiPresentFamily);

  float queuePriority = 1.0f;
  for (nsUInt32 family : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;
    queueInfos.PushBack(queueInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  nsDynamicArray<const char*> extensions = GetRequiredDeviceExtensions();

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<nsUInt32>(queueInfos.GetCount());
  createInfo.pQueueCreateInfos = queueInfos.GetData();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = static_cast<nsUInt32>(extensions.GetCount());
  createInfo.ppEnabledExtensionNames = extensions.GetData();

  static const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
  if (bEnableValidation)
  {
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;
  }

  if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan logical device");
    return NS_FAILURE;
  }

  vkGetDeviceQueue(m_device, indices.m_uiGraphicsFamily, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.m_uiPresentFamily, 0, &m_presentQueue);
  m_uiGraphicsQueueFamily = indices.m_uiGraphicsFamily;
  m_uiPresentQueueFamily = indices.m_uiPresentFamily;
  return NS_SUCCESS;
}
