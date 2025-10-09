#include <Core/CorePCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/System/Screen.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <android_native_app_glue.h>

struct ANativeWindow;

namespace
{
  ANativeWindow* s_androidWindow = nullptr;
  nsEventSubscriptionID s_androidCommandID = 0;
} // namespace

nsWindowAndroid::~nsWindowAndroid()
{
  DestroyWindow();
}

nsResult nsWindowAndroid::InitializeWindow()
{
  NS_LOG_BLOCK("nsWindow::Initialize", m_CreationDescription.m_Title.GetData());
  if (m_bInitialized)
  {
    DestroyWindow();
  }

  if (m_CreationDescription.m_WindowMode == nsWindowMode::WindowResizable)
  {
    s_androidCommandID = nsAndroidUtils::s_AppCommandEvent.AddEventHandler([this](nsInt32 iCmd)
      {
      if (iCmd == APP_CMD_WINDOW_RESIZED)
      {
        nsHybridArray<nsScreenInfo, 2> screens;
        if (nsScreen::EnumerateScreens(screens).Succeeded())
        {
          m_CreationDescription.m_Resolution.width = screens[0].m_iResolutionX;
          m_CreationDescription.m_Resolution.height = screens[0].m_iResolutionY;
          this->OnResize(nsSizeU32(screens[0].m_iResolutionX, screens[0].m_iResolutionY));
        }
      } });
  }

  // Checking and adjustments to creation desc.
  if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
    nsLog::Warning("Failed to adjust window size and position settings.");

  NS_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  NS_ASSERT_RELEASE(s_androidWindow == nullptr, "Window already exists. Only one Android window is supported at any time!");

  s_androidWindow = nsAndroidUtils::GetAndroidApp()->window;
  m_hWindowHandle = s_androidWindow;
  m_pInputDevice = NS_DEFAULT_NEW(nsInputDevice_Android);
  m_bInitialized = true;

  return NS_SUCCESS;
}

void nsWindowAndroid::DestroyWindow()
{
  if (!m_bInitialized)
    return;

  NS_LOG_BLOCK("nsWindow::Destroy");

  s_androidWindow = nullptr;

  if (s_androidCommandID != 0)
  {
    nsAndroidUtils::s_AppCommandEvent.RemoveEventHandler(s_androidCommandID);
  }

  nsLog::Success("Window destroyed.");
}

nsResult nsWindowAndroid::Resize(const nsSizeU32& newWindowSize)
{
  // No need to resize on Android, swapchain can take any size at any time.
  m_CreationDescription.m_Resolution.width = newWindowSize.width;
  m_CreationDescription.m_Resolution.height = newWindowSize.height;
  return NS_SUCCESS;
}

void nsWindowAndroid::ProcessWindowMessages()
{
  NS_ASSERT_RELEASE(s_androidWindow != nullptr, "No window data available.");
}

void nsWindowAndroid::OnResize(const nsSizeU32& newWindowSize)
{
  nsLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

nsWindowHandle nsWindowAndroid::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
