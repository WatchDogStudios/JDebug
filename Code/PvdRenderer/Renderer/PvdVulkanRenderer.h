#pragma once

#include <PvdRenderer/PvdRendererDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/UniquePtr.h>
#include <JVDSDK/Recording/JvdRecordingTypes.h>
#include <VulkanRenderer/VulkanRendererModule.h>

/// \brief Lightweight facade that converts JVD frame data into Vulkan instance buffers and drives the Vulkan renderer.
class NS_PVDRENDERER_DLL nsPvdVulkanRenderer
{
public:
  nsPvdVulkanRenderer();
  ~nsPvdVulkanRenderer();

  nsResult Initialize(const nsVulkanRendererCreateInfo& createInfo);
  void Deinitialize();

  bool IsInitialized() const;

  void SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight);
  void UpdateFrame(const nsJvdFrame& frame);
  nsResult Render(const nsMat4& mViewProjection);

  void SetBodyColorPalette(const nsColor& activeColor, const nsColor& sleepingColor);

private:
  void ConvertFrameToInstances(const nsJvdFrame& frame);

  nsUniquePtr<nsVulkanRenderer> m_pRenderer;
  nsDynamicArray<nsVulkanInstanceData> m_Instances;
  nsColor m_ColorActive;
  nsColor m_ColorSleeping;
};
