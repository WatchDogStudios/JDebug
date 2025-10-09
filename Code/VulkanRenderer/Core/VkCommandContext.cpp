#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/Core/VkCommandContext.h>
#include <VulkanRenderer/Core/VkDevice.h>

#include <volk/volk.h>

#include <Foundation/Logging/Log.h>

nsVkCommandContext::nsVkCommandContext() = default;
nsVkCommandContext::~nsVkCommandContext()
{
  // Deinitialize must be called explicitly with device reference
}

nsResult nsVkCommandContext::Initialize(const nsVkDevice& device, nsUInt32 queueFamily)
{
  NS_ASSERT_DEV(m_commandPool == VK_NULL_HANDLE, "Command pool already created");

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device.GetDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan command pool");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

void nsVkCommandContext::Deinitialize(const nsVkDevice& device)
{
  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(device.GetDevice(), m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }
}
