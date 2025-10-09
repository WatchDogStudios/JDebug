#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Platform/Win/Utils/MinWindows.h>

class NS_CORE_DLL nsInputDeviceMouseKeyboard_Win : public nsInputDeviceMouseKeyboard
{
  NS_ADD_DYNAMIC_REFLECTION(nsInputDeviceMouseKeyboard_Win, nsInputDeviceMouseKeyboard);

public:
  nsInputDeviceMouseKeyboard_Win(nsMinWindows::HWND hWnd);
  ~nsInputDeviceMouseKeyboard_Win();

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(nsMinWindows::UINT msg, nsMinWindows::WPARAM wparam, nsMinWindows::LPARAM lparam);

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  virtual void SetClipMouseCursor(nsMouseCursorClipMode::Enum mode) override;
  virtual nsMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

protected:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void ApplyClipRect(nsMouseCursorClipMode::Enum mode);
  void OnFocusLost();

  nsMinWindows::HWND m_hWnd;
  static nsInputDeviceMouseKeyboard_Win* s_pGlobalInputHandler;
  bool m_bShowCursor = true;
  nsMouseCursorClipMode::Enum m_ClipCursorMode = nsMouseCursorClipMode::NoClip;
  bool m_bApplyClipRect = false;
  // m_bFirstWndMsg and m_bFirstClick are used to fix issues Windows not giving focus to applications that have been launched
  // through a parent process
  bool m_bFirstWndMsg = true;
  bool m_bFirstClick = true;
  nsUInt8 m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  nsUInt8 m_uiMouseButtonReceivedUp[5] = {0, 0, 0, 0, 0};
};
