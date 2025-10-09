#pragma once

#include <VulkanRenderer/Core/VkTypes.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/Types.h>

class nsVkInstance;
class nsVkDevice;

struct NS_VULKANRENDERER_DLL nsVkSwapChainCreateInfo
{
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkExtent2D m_extent = {0, 0};
  nsUInt32 m_uiGraphicsQueueFamily = nsInvalidIndex;
  nsUInt32 m_uiPresentQueueFamily = nsInvalidIndex;
};

class NS_VULKANRENDERER_DLL nsVkSwapChain
{
public:
  nsVkSwapChain();
  ~nsVkSwapChain();

  nsResult Initialize(const nsVkInstance& instance, const nsVkDevice& device, const nsVkSwapChainCreateInfo& createInfo);
  void Deinitialize(const nsVkDevice& device);

  VkSwapchainKHR GetSwapChain() const { return m_swapChain; }
  VkFormat GetImageFormat() const { return m_imageFormat; }
  const nsDynamicArray<VkImageView>& GetImageViews() const { return m_imageViews; }
  VkExtent2D GetExtent() const { return m_extent; }
  nsUInt32 GetImageCount() const { return m_images.GetCount(); }
  VkImage GetImage(nsUInt32 uiIndex) const
  {
    NS_ASSERT_DEV(uiIndex < m_images.GetCount(), "Invalid swap-chain image index");
    return m_images[uiIndex];
  }

private:
  nsResult CreateSwapChain(const nsVkInstance& instance, const nsVkDevice& device, const nsVkSwapChainCreateInfo& createInfo);
  nsResult CreateImageViews(const nsVkDevice& device);

  VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
  VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
  nsDynamicArray<VkImage> m_images;
  nsDynamicArray<VkImageView> m_imageViews;
  VkExtent2D m_extent = {0, 0};
};
