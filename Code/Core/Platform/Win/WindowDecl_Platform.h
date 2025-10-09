
#if NS_ENABLED(NS_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

using nsWindowHandle = nsMinWindows::HWND;
using nsWindowInternalHandle = GLFWwindow*;
#  define INVALID_WINDOW_HANDLE_VALUE (nsWindowHandle)(0)
#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE nullptr

#else

#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <InputDevice_Platform.h>

using nsWindowHandle = nsMinWindows::HWND;
using nsWindowInternalHandle = nsWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE (nsWindowHandle)(0)

#endif
