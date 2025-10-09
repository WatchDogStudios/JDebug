#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

#if NS_ENABLED(NS_SUPPORTS_GLFW)

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

class NS_CORE_DLL nsInputDeviceMouseKeyboard_GLFW : public nsInputDeviceMouseKeyboard
{
  NS_ADD_DYNAMIC_REFLECTION(nsInputDeviceMouseKeyboard_GLFW, nsInputDeviceMouseKeyboard);

public:
  nsInputDeviceMouseKeyboard_GLFW(GLFWwindow* windowHandle);
  ~nsInputDeviceMouseKeyboard_GLFW();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(nsMouseCursorClipMode::Enum mode) override;
  virtual nsMouseCursorClipMode::Enum GetClipMouseCursor() const override;

  // GLFW callback for key pressed, released, repeated events
  void OnKey(int key, int scancode, int action, int mods);

  // GLFW callback for text input (each UTF32 code point individually)
  void OnCharacter(unsigned int codepoint);

  // GLFW callback on mouse move
  void OnCursorPosition(double xpos, double ypos);

  // GLFW callback on mouse button actions
  void OnMouseButton(int button, int action, int mods);

  // GLFW callback for mouse scroll
  void OnScroll(double xoffset, double yoffset);

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

private:
  GLFWwindow* m_pWindow = nullptr;
  nsVec2d m_LastPos = nsVec2d(nsMath::MaxValue<double>());
};

#endif
