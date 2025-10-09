#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class NS_CORE_DLL nsInputDeviceMouseKeyboard_NoImpl : public nsInputDeviceMouseKeyboard
{
  NS_ADD_DYNAMIC_REFLECTION(nsInputDeviceMouseKeyboard_NoImpl, nsInputDeviceMouseKeyboard);

public:
  nsInputDeviceMouseKeyboard_NoImpl(nsUInt32 uiWindowNumber);
  ~nsInputDeviceMouseKeyboard_NoImpl();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(nsMouseCursorClipMode::Enum mode) override;
  virtual nsMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};
