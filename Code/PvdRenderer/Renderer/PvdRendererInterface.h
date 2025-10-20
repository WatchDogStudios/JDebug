#pragma once

#include <PvdRenderer/PvdRendererDLL.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Color.h>
#include <JVDSDK/Recording/JvdRecordingTypes.h>

/// \brief Enum to specify which renderer backend to use
enum class nsPvdRendererType
{
  Vulkan,
  DirectX11
};

/// \brief Abstract interface for PVD renderers to allow switching between backends
class NS_PVDRENDERER_DLL nsPvdRendererInterface
{
public:
  virtual ~nsPvdRendererInterface() = default;

  /// \brief Returns the type of this renderer
  virtual nsPvdRendererType GetRendererType() const = 0;

  /// \brief Check if the renderer is currently initialized
  virtual bool IsInitialized() const = 0;

  /// \brief Update the backbuffer size (e.g., on window resize)
  virtual void SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight) = 0;

  /// \brief Update the frame data to be visualized
  virtual void UpdateFrame(const nsJvdFrame& frame) = 0;

  /// \brief Render the current frame with the given view-projection matrix
  virtual nsResult Render(const nsMat4& mViewProjection) = 0;

  /// \brief Set the color palette for active/sleeping bodies
  virtual void SetBodyColorPalette(const nsColor& activeColor, const nsColor& sleepingColor) = 0;

  /// \brief Deinitialize the renderer
  virtual void Deinitialize() = 0;
};
