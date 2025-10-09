#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/Core/VkInstance.h>

#include <volk/volk.h>

#include <Foundation/Logging/Log.h>

nsVkInstance::nsVkInstance() = default;
nsVkInstance::~nsVkInstance()
{
  Deinitialize();
}

nsResult nsVkInstance::Initialize(const nsVkInstanceCreateInfo& createInfo)
{
  NS_ASSERT_DEV(m_instance == VK_NULL_HANDLE, "Instance already created");

  if (volkInitialize() != VK_SUCCESS)
  {
    nsLog::Error("Failed to initialize volk Vulkan loader");
    return NS_FAILURE;
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = createInfo.m_sApplicationName.GetData();
  appInfo.applicationVersion = createInfo.m_uiApplicationVersion;
  appInfo.pEngineName = "JDebug";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  info.pApplicationInfo = &appInfo;
  info.enabledExtensionCount = static_cast<nsUInt32>(createInfo.m_Extensions.GetCount());
  info.ppEnabledExtensionNames = createInfo.m_Extensions.GetData();

  VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
  if (createInfo.m_bEnableValidation)
  {
    static const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = validationLayers;

    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void*) -> VkBool32 {
      switch (severity)
      {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
          nsLog::Error("[Vulkan] {}", callbackData->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
          nsLog::Warning("[Vulkan] {}", callbackData->pMessage);
          break;
        default:
          nsLog::Info("[Vulkan] {}", callbackData->pMessage);
          break;
      }
      return VK_FALSE;
    };
    info.pNext = &debugInfo;
  }

  if (vkCreateInstance(&info, nullptr, &m_instance) != VK_SUCCESS)
  {
    nsLog::Error("Failed to create Vulkan instance");
    return NS_FAILURE;
  }

  volkLoadInstance(m_instance);
  return NS_SUCCESS;
}

void nsVkInstance::Deinitialize()
{
  if (m_instance != VK_NULL_HANDLE)
  {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }
}
