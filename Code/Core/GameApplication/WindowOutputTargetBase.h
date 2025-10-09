#pragma once

#include <Core/CoreDLL.h>

class nsImage;

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa nsWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by nsGameApplication to work with
/// the render output.
class NS_CORE_DLL nsWindowOutputTargetBase
{
public:
  virtual ~nsWindowOutputTargetBase() = default;
  virtual void AcquireImage() = 0;
  virtual void PresentImage(bool bEnableVSync) = 0;
  virtual nsResult CaptureImage(nsImage& out_image) = 0;
};
