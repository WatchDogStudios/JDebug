
#if NS_ENABLED(NS_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

extern "C"
{
  typedef struct xcb_connection_t xcb_connection_t;
}

struct nsXcbWindowHandle
{
  xcb_connection_t* m_pConnection;
  nsUInt32 m_Window;
};

struct nsWindowHandle
{
  enum class Type
  {
    Invalid = 0,
    GLFW = 1, // Used by the runtime
    XCB = 2   // Used by the editor
  };

  Type type;
  union
  {
    GLFWwindow* glfwWindow;
    nsXcbWindowHandle xcbWindow;
  };

  bool operator==(nsWindowHandle& rhs)
  {
    if (type != rhs.type)
      return false;

    if (type == Type::GLFW)
    {
      return glfwWindow == rhs.glfwWindow;
    }
    else
    {
      // We don't compare the connection because we only want to know if we reference the same window.
      return xcbWindow.m_Window == rhs.xcbWindow.m_Window;
    }
  }
};

using nsWindowInternalHandle = nsWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE \
    nsWindowHandle {}

#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE INVALID_WINDOW_HANDLE_VALUE

#else

#  error "Linux has no native window support"

#endif
