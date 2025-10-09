#include <Core/CorePCH.h>

#if NS_ENABLED(NS_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Configuration/Startup.h>

#  include <GLFW/glfw3.h>

#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
#    ifdef APIENTRY
#      undef APIENTRY
#    endif

#    include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#  endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    nsLog::Error("GLFW error {}: {}", errorCode, msg);
  }
} // namespace


// clang-format off
NS_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      nsLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    else
    {
      // Set the error callback after init, so we don't print an error if init fails.
      glfwSetErrorCallback(&glfwErrorCallback);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    glfwSetErrorCallback(nullptr);
    glfwTerminate();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

NS_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  nsResult nsGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if (errorCode != GLFW_NO_ERROR)
    {
      nsLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return NS_FAILURE;
    }
    return NS_SUCCESS;
  }
} // namespace

#  define NS_GLFW_RETURN_FAILURE_ON_ERROR()         \
    do                                              \
    {                                               \
      if (nsGlfwError(__FILE__, __LINE__).Failed()) \
        return NS_FAILURE;                          \
    } while (false)

nsWindowGLFW::~nsWindowGLFW()
{
  DestroyWindow();
}

nsResult nsWindowGLFW::InitializeWindow()
{
  NS_LOG_BLOCK("nsWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    DestroyWindow();
  }

  NS_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  GLFWmonitor* pMonitor = nullptr; // nullptr for windowed, fullscreen otherwise

  switch (m_CreationDescription.m_WindowMode)
  {
    case nsWindowMode::WindowResizable:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      NS_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case nsWindowMode::WindowFixedResolution:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      NS_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case nsWindowMode::FullscreenFixedResolution:
    case nsWindowMode::FullscreenBorderlessNativeResolution:
      if (m_CreationDescription.m_iMonitor == -1)
      {
        pMonitor = glfwGetPrimaryMonitor();
        NS_GLFW_RETURN_FAILURE_ON_ERROR();
      }
      else
      {
        int iMonitorCount = 0;
        GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
        NS_GLFW_RETURN_FAILURE_ON_ERROR();
        if (m_CreationDescription.m_iMonitor >= iMonitorCount)
        {
          nsLog::Error("Can not create window on monitor {} only {} monitors connected", m_CreationDescription.m_iMonitor, iMonitorCount);
          return NS_FAILURE;
        }
        pMonitor = pMonitors[m_CreationDescription.m_iMonitor];
      }

      if (m_CreationDescription.m_WindowMode == nsWindowMode::FullscreenBorderlessNativeResolution)
      {
        const GLFWvidmode* pVideoMode = glfwGetVideoMode(pMonitor);
        NS_GLFW_RETURN_FAILURE_ON_ERROR();
        if (pVideoMode == nullptr)
        {
          nsLog::Error("Failed to get video mode for monitor");
          return NS_FAILURE;
        }
        m_CreationDescription.m_Resolution.width = pVideoMode->width;
        m_CreationDescription.m_Resolution.height = pVideoMode->height;
        m_CreationDescription.m_Position.x = 0;
        m_CreationDescription.m_Position.y = 0;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        NS_GLFW_RETURN_FAILURE_ON_ERROR();
      }

      break;
  }


  glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_CreationDescription.m_bSetForegroundOnInit ? GLFW_TRUE : GLFW_FALSE);
  NS_GLFW_RETURN_FAILURE_ON_ERROR();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  NS_GLFW_RETURN_FAILURE_ON_ERROR();

  GLFWwindow* pWindow = glfwCreateWindow(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, m_CreationDescription.m_Title.GetData(), pMonitor, NULL);
  NS_GLFW_RETURN_FAILURE_ON_ERROR();

  if (pWindow == nullptr)
  {
    nsLog::Error("Failed to create glfw window");
    return NS_FAILURE;
  }
#  if NS_ENABLED(NS_PLATFORM_LINUX)
  m_hWindowHandle.type = nsWindowHandle::Type::GLFW;
  m_hWindowHandle.glfwWindow = pWindow;
#  else
  m_hWindowHandle = pWindow;
#  endif

  if (m_CreationDescription.m_Position != nsVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    NS_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowIconifyCallback(pWindow, &nsWindowGLFW::IconifyCallback);
  glfwSetWindowSizeCallback(pWindow, &nsWindowGLFW::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &nsWindowGLFW::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &nsWindowGLFW::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &nsWindowGLFW::FocusCallback);
  glfwSetKeyCallback(pWindow, &nsWindowGLFW::KeyCallback);
  glfwSetCharCallback(pWindow, &nsWindowGLFW::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &nsWindowGLFW::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &nsWindowGLFW::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &nsWindowGLFW::ScrollCallback);
  NS_GLFW_RETURN_FAILURE_ON_ERROR();

#  if NS_ENABLED(NS_PLATFORM_LINUX)
  NS_ASSERT_DEV(m_hWindowHandle.type == nsWindowHandle::Type::GLFW, "not a GLFW handle");
  auto pInput = NS_DEFAULT_NEW(nsInputDeviceMouseKeyboard_GLFW, m_hWindowHandle.glfwWindow);
#  else
  auto pInput = NS_DEFAULT_NEW(nsInputDeviceMouseKeyboard_Win, m_hWindowHandle);
#  endif

  pInput->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? nsMouseCursorClipMode::ClipToWindowImmediate : nsMouseCursorClipMode::NoClip);
  pInput->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_pInputDevice = std::move(pInput);

  m_bInitialized = true;
  nsLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return NS_SUCCESS;
}

void nsWindowGLFW::DestroyWindow()
{
  if (m_bInitialized)
  {
    NS_LOG_BLOCK("nsWindow::Destroy");

    m_pInputDevice = nullptr;

#  if NS_ENABLED(NS_PLATFORM_LINUX)
    NS_ASSERT_DEV(m_hWindowHandle.type == nsWindowHandle::Type::GLFW, "GLFW handle expected");
    glfwDestroyWindow(m_hWindowHandle.glfwWindow);
#  else
    glfwDestroyWindow(m_hWindowHandle);
#  endif
    m_hWindowHandle = INVALID_INTERNAL_WINDOW_HANDLE_VALUE;

    m_bInitialized = false;
  }
}

nsResult nsWindowGLFW::Resize(const nsSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return NS_FAILURE;

#  if NS_ENABLED(NS_PLATFORM_LINUX)
  NS_ASSERT_DEV(m_hWindowHandle.type == nsWindowHandle::Type::GLFW, "Expected GLFW handle");
  glfwSetWindowSize(m_hWindowHandle.glfwWindow, newWindowSize.width, newWindowSize.height);
#  else
  glfwSetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#  endif
  NS_GLFW_RETURN_FAILURE_ON_ERROR();

  return NS_SUCCESS;
}

void nsWindowGLFW::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  // if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

#  if NS_ENABLED(NS_PLATFORM_LINUX)
  NS_ASSERT_DEV(m_hWindowHandle.type == nsWindowHandle::Type::GLFW, "Expected GLFW handle");
  if (glfwWindowShouldClose(m_hWindowHandle.glfwWindow))
  {
    DestroyWindow();
  }
#  else
  if (glfwWindowShouldClose(m_hWindowHandle))
  {
    DestroyWindow();
  }
#  endif
}

void nsWindowGLFW::OnResize(const nsSizeU32& newWindowSize)
{
  nsLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void nsWindowGLFW::IconifyCallback(GLFWwindow* window, int iconified)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
    self->OnVisibleChange(!iconified);
}

void nsWindowGLFW::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(nsSizeU32(static_cast<nsUInt32>(width), static_cast<nsUInt32>(height)));
  }
}

void nsWindowGLFW::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void nsWindowGLFW::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void nsWindowGLFW::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void nsWindowGLFW::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = nsDynamicCast<nsInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnKey(key, scancode, action, mods);
    }
  }
}

void nsWindowGLFW::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = nsDynamicCast<nsInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCharacter(codepoint);
    }
  }
}

void nsWindowGLFW::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = nsDynamicCast<nsInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCursorPosition(xpos, ypos);
    }
  }
}

void nsWindowGLFW::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = nsDynamicCast<nsInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnMouseButton(button, action, mods);
    }
  }
}

void nsWindowGLFW::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<nsWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = nsDynamicCast<nsInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnScroll(xoffset, yoffset);
    }
  }
}

nsWindowHandle nsWindowGLFW::GetNativeWindowHandle() const
{
#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
  return nsMinWindows::FromNative<HWND>(glfwGetWin32Window(m_hWindowHandle));
#  else
  return m_hWindowHandle;
#  endif
}

#endif


NS_STATICLINK_FILE(Core, Core_Platform_GLFW_Window_GLFW);
