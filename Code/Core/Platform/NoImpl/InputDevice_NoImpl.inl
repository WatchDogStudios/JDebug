#include <Core/Platform/NoImpl/InputDevice_NoImpl.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsInputDeviceMouseKeyboard_NoImpl, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

nsInputDeviceMouseKeyboard_NoImpl::nsInputDeviceMouseKeyboard_NoImpl(nsUInt32 uiWindowNumber) {}
nsInputDeviceMouseKeyboard_NoImpl::~nsInputDeviceMouseKeyboard_NoImpl() = default;

void nsInputDeviceMouseKeyboard_NoImpl::SetShowMouseCursor(bool bShow) {}

bool nsInputDeviceMouseKeyboard_NoImpl::GetShowMouseCursor() const
{
  return false;
}

void nsInputDeviceMouseKeyboard_NoImpl::SetClipMouseCursor(nsMouseCursorClipMode::Enum mode) {}

nsMouseCursorClipMode::Enum nsInputDeviceMouseKeyboard_NoImpl::GetClipMouseCursor() const
{
  return nsMouseCursorClipMode::Default;
}

void nsInputDeviceMouseKeyboard_NoImpl::InitializeDevice() {}

void nsInputDeviceMouseKeyboard_NoImpl::RegisterInputSlots() {}
