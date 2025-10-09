#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/Core/VkSwapChain.h>
#include <VulkanRenderer/Core/VkInstance.h>
#include <VulkanRenderer/Core/VkDevice.h>

#include <volk/volk.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Strings/FormatString.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  undef max
#  undef min
#endif

#include <limits>

namespace
{
  struct SwapChainSupportDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    nsDynamicArray<VkSurfaceFormatKHR> formats;
    nsDynamicArray<VkPresentModeKHR> presentModes;
  };

  SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
  {
    SwapChainSupportDetails details;

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    if (result != VK_SUCCESS)
    {
  nsLog::Error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed (VkResult: {0})", nsArgI(static_cast<nsInt32>(result)));
      return details;
    }

    nsUInt32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
      details.formats.SetCount(formatCount);
      result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.GetData());
      if (result != VK_SUCCESS)
      {
  nsLog::Error("vkGetPhysicalDeviceSurfaceFormatsKHR failed (VkResult: {0})", nsArgI(static_cast<nsInt32>(result)));
        details.formats.Clear();
      }
    }

    nsUInt32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
      details.presentModes.SetCount(presentModeCount);
      result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.GetData());
      if (result != VK_SUCCESS)
      {
  nsLog::Error("vkGetPhysicalDeviceSurfacePresentModesKHR failed (VkResult: {0})", nsArgI(static_cast<nsInt32>(result)));
        details.presentModes.Clear();
      }
    }

    return details;
  }

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const nsDynamicArray<VkSurfaceFormatKHR>& formats)
  {
    if (formats.IsEmpty())
      return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    if (formats.GetCount() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
      return {VK_FORMAT_B8G8R8A8_UNORM, formats[0].colorSpace};

    for (const auto& available : formats)
    {
      if (available.format == VK_FORMAT_B8G8R8A8_SRGB && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return available;
    }

    VkSurfaceFormatKHR fallback = formats[0];
    if (fallback.format == VK_FORMAT_UNDEFINED)
    {
      fallback.format = VK_FORMAT_B8G8R8A8_UNORM;
      if (fallback.colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT)
      {
        // DISPLAY_P3 is uncommon; default to sRGB for better compatibility if driver reported undefined format.
        fallback.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      }
    }
    return fallback;
  }

  VkPresentModeKHR ChoosePresentMode(const nsDynamicArray<VkPresentModeKHR>& presentModes)
  {
    if (presentModes.IsEmpty())
      return VK_PRESENT_MODE_FIFO_KHR;

    for (VkPresentModeKHR mode : presentModes)
    {
      if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D requested)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<nsUInt32>::max())
      return capabilities.currentExtent;

    VkExtent2D actualExtent = requested;
    actualExtent.width = nsMath::Clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = nsMath::Clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
  }
}

nsVkSwapChain::nsVkSwapChain() = default;
nsVkSwapChain::~nsVkSwapChain() = default;

nsResult nsVkSwapChain::Initialize(const nsVkInstance& instance, const nsVkDevice& device, const nsVkSwapChainCreateInfo& createInfo)
{
  NS_ASSERT_DEV(m_swapChain == VK_NULL_HANDLE, "Swapchain already created");

  if (CreateSwapChain(instance, device, createInfo).Failed())
    return NS_FAILURE;

  if (CreateImageViews(device).Failed())
    return NS_FAILURE;

  return NS_SUCCESS;
}

void nsVkSwapChain::Deinitialize(const nsVkDevice& device)
{
  for (VkImageView view : m_imageViews)
  {
    vkDestroyImageView(device.GetDevice(), view, nullptr);
  }
  m_imageViews.Clear();
  m_images.Clear();

  if (m_swapChain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(device.GetDevice(), m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
  }

  m_extent = {0, 0};
}

nsResult nsVkSwapChain::CreateSwapChain(const nsVkInstance& instance, const nsVkDevice& device, const nsVkSwapChainCreateInfo& createInfo)
{
  NS_ASSERT_DEV(createInfo.m_uiGraphicsQueueFamily != nsInvalidIndex, "Graphics queue family must be specified for swapchain creation");
  NS_ASSERT_DEV(createInfo.m_uiPresentQueueFamily != nsInvalidIndex, "Present queue family must be specified for swapchain creation");

  SwapChainSupportDetails support = QuerySwapChainSupport(device.GetPhysicalDevice(), createInfo.m_surface);

  if (support.formats.IsEmpty())
  {
    nsLog::Error("No Vulkan surface formats available for swapchain creation");
    return NS_FAILURE;
  }

  if (support.presentModes.IsEmpty())
  {
    nsLog::Error("No Vulkan present modes available for swapchain creation");
    return NS_FAILURE;
  }

  VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
  VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
  VkExtent2D extent = ChooseSwapExtent(support.capabilities, createInfo.m_extent);

  nsUInt32 imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    imageCount = support.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR swapInfo = {};
  swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapInfo.surface = createInfo.m_surface;
  swapInfo.minImageCount = imageCount;
  swapInfo.imageFormat = surfaceFormat.format;
  swapInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapInfo.imageExtent = extent;
  swapInfo.imageArrayLayers = 1;
  swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const nsUInt32 queueFamilyIndices[] = {createInfo.m_uiGraphicsQueueFamily, createInfo.m_uiPresentQueueFamily};
  if (createInfo.m_uiGraphicsQueueFamily != createInfo.m_uiPresentQueueFamily)
  {
    swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapInfo.queueFamilyIndexCount = 2;
    swapInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.queueFamilyIndexCount = 0;
    swapInfo.pQueueFamilyIndices = nullptr;
  }
  swapInfo.preTransform = support.capabilities.currentTransform;
  const VkCompositeAlphaFlagBitsKHR compositeAlphaPriorities[] = {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

  VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  for (VkCompositeAlphaFlagBitsKHR candidate : compositeAlphaPriorities)
  {
    if (support.capabilities.supportedCompositeAlpha & candidate)
    {
      compositeAlpha = candidate;
      break;
    }
  }
  swapInfo.compositeAlpha = compositeAlpha;
  swapInfo.presentMode = presentMode;
  swapInfo.clipped = VK_TRUE;
  swapInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult createResult = vkCreateSwapchainKHR(device.GetDevice(), &swapInfo, nullptr, &m_swapChain);
  if (createResult != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan swapchain (VkResult: {0}, extent: {1}x{2}, imageCount: {3}, compositeAlpha: 0x{4})",
      nsArgI(static_cast<nsInt32>(createResult)),
      nsArgU(extent.width),
      nsArgU(extent.height),
      nsArgU(imageCount),
      nsArgU(static_cast<nsUInt32>(compositeAlpha)));
    return NS_FAILURE;
  }

  VkResult imageResult = vkGetSwapchainImagesKHR(device.GetDevice(), m_swapChain, &imageCount, nullptr);
  if (imageResult != VK_SUCCESS || imageCount == 0)
  {
  nsLog::Error("Failed to query Vulkan swapchain images (VkResult: {0})", nsArgI(static_cast<nsInt32>(imageResult)));
    return NS_FAILURE;
  }
  m_images.SetCount(imageCount);
  imageResult = vkGetSwapchainImagesKHR(device.GetDevice(), m_swapChain, &imageCount, m_images.GetData());
  if (imageResult != VK_SUCCESS)
  {
  nsLog::Error("Failed to retrieve Vulkan swapchain images (VkResult: {0})", nsArgI(static_cast<nsInt32>(imageResult)));
    return NS_FAILURE;
  }

  m_imageFormat = surfaceFormat.format;
  m_extent = extent;
  return NS_SUCCESS;
}

nsResult nsVkSwapChain::CreateImageViews(const nsVkDevice& device)
{
  m_imageViews.SetCount(m_images.GetCount());

  for (nsUInt32 i = 0; i < m_images.GetCount(); ++i)
  {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_imageFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
    {
      nsLog::Error("Failed to create Vulkan swapchain image view");
      return NS_FAILURE;
    }
  }

  return NS_SUCCESS;
}
