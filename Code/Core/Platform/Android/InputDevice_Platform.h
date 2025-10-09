#pragma once

#include <Core/Input/InputDevice.h>

struct nsAndroidInputEvent;
struct AInputEvent;

/// \brief Android standard input device.
class NS_CORE_DLL nsInputDevice_Android : public nsInputDevice
{
  NS_ADD_DYNAMIC_REFLECTION(nsInputDevice_Android, nsInputDevice);

public:
  nsInputDevice_Android();
  ~nsInputDevice_Android();

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void AndroidInputEventHandler(nsAndroidInputEvent& event);
  void AndroidAppCommandEventHandler(nsInt32 iCmd);
  bool AndroidHandleInput(AInputEvent* pEvent);

private:
  nsInt32 m_iResolutionX = 0;
  nsInt32 m_iResolutionY = 0;
};
