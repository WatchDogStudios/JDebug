#include <Core/CorePCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)

#  include <Core/Platform/Android/InputDevice_Platform.h>

#  include <Core/Input/InputManager.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/System/Screen.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsInputDevice_Android, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// Comment in to get verbose output on android input
// #  define DEBUG_ANDROID_INPUT

#  ifdef DEBUG_ANDROID_INPUT
#    define DEBUG_LOG(...) nsLog::Debug(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

nsInputDevice_Android::nsInputDevice_Android()
{
  nsAndroidUtils::s_InputEvent.AddEventHandler(nsMakeDelegate(&nsInputDevice_Android::AndroidInputEventHandler, this));
  nsAndroidUtils::s_AppCommandEvent.AddEventHandler(nsMakeDelegate(&nsInputDevice_Android::AndroidAppCommandEventHandler, this));
}

nsInputDevice_Android::~nsInputDevice_Android()
{
  nsAndroidUtils::s_AppCommandEvent.RemoveEventHandler(nsMakeDelegate(&nsInputDevice_Android::AndroidAppCommandEventHandler, this));
  nsAndroidUtils::s_InputEvent.RemoveEventHandler(nsMakeDelegate(&nsInputDevice_Android::AndroidInputEventHandler, this));
}

void nsInputDevice_Android::InitializeDevice()
{
  nsHybridArray<nsScreenInfo, 2> screens;
  if (nsScreen::EnumerateScreens(screens).Succeeded())
  {
    m_iResolutionX = screens[0].m_iResolutionX;
    m_iResolutionY = screens[0].m_iResolutionY;
  }
}

void nsInputDevice_Android::UpdateInputSlotValues()
{
  // nothing to do here
}

void nsInputDevice_Android::RegisterInputSlots()
{
  RegisterInputSlot(nsInputSlot_TouchPoint0, "Touchpoint 0", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint0_PositionX, "Touchpoint 0 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint0_PositionY, "Touchpoint 0 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint1, "Touchpoint 1", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint1_PositionX, "Touchpoint 1 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint1_PositionY, "Touchpoint 1 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint2, "Touchpoint 2", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint2_PositionX, "Touchpoint 2 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint2_PositionY, "Touchpoint 2 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint3, "Touchpoint 3", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint3_PositionX, "Touchpoint 3 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint3_PositionY, "Touchpoint 3 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint4, "Touchpoint 4", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint4_PositionX, "Touchpoint 4 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint4_PositionY, "Touchpoint 4 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint5, "Touchpoint 5", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint5_PositionX, "Touchpoint 5 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint5_PositionY, "Touchpoint 5 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint6, "Touchpoint 6", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint6_PositionX, "Touchpoint 6 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint6_PositionY, "Touchpoint 6 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint7, "Touchpoint 7", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint7_PositionX, "Touchpoint 7 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint7_PositionY, "Touchpoint 7 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint8, "Touchpoint 8", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint8_PositionX, "Touchpoint 8 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint8_PositionY, "Touchpoint 8 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_TouchPoint9, "Touchpoint 9", nsInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(nsInputSlot_TouchPoint9_PositionX, "Touchpoint 9 Position X", nsInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(nsInputSlot_TouchPoint9_PositionY, "Touchpoint 9 Position Y", nsInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(nsInputSlot_MouseWheelUp, "Mousewheel Up", nsInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(nsInputSlot_MouseWheelDown, "Mousewheel Down", nsInputSlotFlags::IsMouseWheel);
}

void nsInputDevice_Android::ResetInputSlotValues()
{
  m_InputSlotValues[nsInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[nsInputSlot_MouseWheelDown] = 0;
  for (int id = 0; id < 10; ++id)
  {
    // We can't reset the position inside AndroidHandleInput as we want the position to be valid when lifting a finger. Thus, we clear the position here after the update has been performed.
    if (m_InputSlotValues[nsInputManager::GetInputSlotTouchPoint(id)] == 0)
    {
      m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionX(id)] = 0;
      m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionY(id)] = 0;
    }
  }
}

void nsInputDevice_Android::AndroidInputEventHandler(nsAndroidInputEvent& event)
{
  event.m_bHandled = AndroidHandleInput(event.m_pEvent);
  UpdateInputSlotValues();
}

void nsInputDevice_Android::AndroidAppCommandEventHandler(nsInt32 iCmd)
{
  if (iCmd == APP_CMD_WINDOW_RESIZED)
  {
    nsHybridArray<nsScreenInfo, 2> screens;
    if (nsScreen::EnumerateScreens(screens).Succeeded())
    {
      m_iResolutionX = screens[0].m_iResolutionX;
      m_iResolutionY = screens[0].m_iResolutionY;
    }
  }
}

bool nsInputDevice_Android::AndroidHandleInput(AInputEvent* pEvent)
{
  // #TODO_ANDROID Only touchscreen input is implemented right now.
  const nsInt32 iEventType = AInputEvent_getType(pEvent);
  const nsInt32 iEventSource = AInputEvent_getSource(pEvent);
  const nsUInt32 uiAction = (nsUInt32)AMotionEvent_getAction(pEvent);
  const nsInt32 iKeyCode = AKeyEvent_getKeyCode(pEvent);
  const nsInt32 iButtonState = AMotionEvent_getButtonState(pEvent);
  NS_IGNORE_UNUSED(iKeyCode);
  NS_IGNORE_UNUSED(iButtonState);
  DEBUG_LOG("Android INPUT: iEventType: {}, iEventSource: {}, uiAction: {}, iKeyCode: {}, iButtonState: {}", iEventType,
    iEventSource, uiAction, iKeyCode, iButtonState);

  if (m_iResolutionX == 0 || m_iResolutionY == 0)
    return false;

  // I.e. fingers have touched the touchscreen.
  if (iEventType == AINPUT_EVENT_TYPE_MOTION && (iEventSource & AINPUT_SOURCE_TOUCHSCREEN) != 0)
  {
    // Update pointer positions
    const nsUInt64 uiPointerCount = AMotionEvent_getPointerCount(pEvent);
    for (nsUInt32 uiPointerIndex = 0; uiPointerIndex < uiPointerCount; uiPointerIndex++)
    {
      const float fPixelX = AMotionEvent_getX(pEvent, uiPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiPointerIndex);
      const nsInt32 id = AMotionEvent_getPointerId(pEvent, uiPointerIndex);
      if (id < 10)
      {
        m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
        m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
        DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionY(id)]);
      }
    }

    // Update pointer state
    const nsUInt32 uiActionEvent = uiAction & AMOTION_EVENT_ACTION_MASK;
    const nsUInt32 uiActionPointerIndex = (uiAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const nsInt32 id = AMotionEvent_getPointerId(pEvent, uiActionPointerIndex);
    // We only support up to 10 touch points at the same time.
    if (id >= 10)
      return false;

    {
      // Not sure if the action finger is always present in the upper loop of uiPointerCount, so we update it here for good measure.
      const float fPixelX = AMotionEvent_getX(pEvent, uiActionPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiActionPointerIndex);
      m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
      m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
      DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[nsInputManager::GetInputSlotTouchPointPositionY(id)]);
    }

    switch (uiActionEvent)
    {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        m_InputSlotValues[nsInputManager::GetInputSlotTouchPoint(id)] = 1;
        DEBUG_LOG("Finger DOWN: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_MOVE:
        // Finger moved (we always update that at the top).
        return true;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
      case AMOTION_EVENT_ACTION_CANCEL:
      case AMOTION_EVENT_ACTION_OUTSIDE:
        m_InputSlotValues[nsInputManager::GetInputSlotTouchPoint(id)] = 0;
        DEBUG_LOG("Finger UP: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_SCROLL:
      {
        float fRotated = AMotionEvent_getAxisValue(pEvent, AMOTION_EVENT_AXIS_VSCROLL, 0);
        if (fRotated > 0)
          m_InputSlotValues[nsInputSlot_MouseWheelUp] = fRotated;
        else
          m_InputSlotValues[nsInputSlot_MouseWheelDown] = fRotated;
        return true;
      }
      case AMOTION_EVENT_ACTION_HOVER_ENTER:
      case AMOTION_EVENT_ACTION_HOVER_MOVE:
      case AMOTION_EVENT_ACTION_HOVER_EXIT:
        return false;
      default:
        DEBUG_LOG("Unknown AMOTION_EVENT_ACTION: {}", uiActionEvent);
        return false;
    }
  }
  return false;
}

#endif


NS_STATICLINK_FILE(Core, Core_Platform_Android_InputDevice_Android);
