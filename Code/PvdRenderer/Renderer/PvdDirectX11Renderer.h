#pragma once

#include <PvdRenderer/PvdRendererDLL.h>
#include <PvdRenderer/Renderer/PvdRendererInterface.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/UniquePtr.h>
#include <JVDSDK/Recording/JvdRecordingTypes.h>
#include <DirectX11Renderer/DirectX11RendererModule.h>

/// \brief Lightweight facade that converts JVD frame data into DirectX11 instance buffers and drives the DirectX11 renderer.
class NS_PVDRENDERER_DLL nsPvdDirectX11Renderer : public nsPvdRendererInterface
{
public:
  nsPvdDirectX11Renderer();
  ~nsPvdDirectX11Renderer();

  nsResult Initialize(const nsDirectX11RendererCreateInfo& createInfo);

  // nsPvdRendererInterface implementation
  virtual nsPvdRendererType GetRendererType() const override { return nsPvdRendererType::DirectX11; }
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;
  virtual void SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight) override;
  virtual void UpdateFrame(const nsJvdFrame& frame) override;
  virtual nsResult Render(const nsMat4& mViewProjection) override;
  virtual void SetBodyColorPalette(const nsColor& activeColor, const nsColor& sleepingColor) override;

private:
  void ConvertFrameToInstances(const nsJvdFrame& frame);

  nsUniquePtr<nsDirectX11Renderer> m_pRenderer;
  nsDynamicArray<nsDirectX11InstanceData> m_Instances;
  nsColor m_ColorActive;
  nsColor m_ColorSleeping;
};
