#pragma once

#include <VulkanRenderer/VulkanRendererDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/TypeTraits.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

NS_DEFINE_AS_POD_TYPE(VkQueueFamilyProperties);
NS_DEFINE_AS_POD_TYPE(VkExtensionProperties);
NS_DEFINE_AS_POD_TYPE(VkDeviceQueueCreateInfo);
NS_DEFINE_AS_POD_TYPE(VkSurfaceFormatKHR);
NS_DEFINE_AS_POD_TYPE(VkPresentModeKHR);
NS_DEFINE_AS_POD_TYPE(VkExtent2D);
NS_DEFINE_AS_POD_TYPE(VkRect2D);
NS_DEFINE_AS_POD_TYPE(VkViewport);
NS_DEFINE_AS_POD_TYPE(VkDescriptorSetLayoutBinding);
