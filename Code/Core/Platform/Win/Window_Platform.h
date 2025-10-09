
#if NS_ENABLED(NS_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/Window_GLFW.h>

#else

class NS_CORE_DLL nsWindowWin : public nsWindowPlatformShared
{
public:
  ~nsWindowWin();

  virtual nsResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual nsResult Resize(const nsSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual void OnResize(const nsSizeU32& newWindowSize) override;
  virtual nsWindowHandle GetNativeWindowHandle() const override;

  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(nsMinWindows::HWND hWnd, nsMinWindows::UINT msg, nsMinWindows::WPARAM wparam, nsMinWindows::LPARAM lparam)
  {
    NS_IGNORE_UNUSED(hWnd);
    NS_IGNORE_UNUSED(msg);
    NS_IGNORE_UNUSED(wparam);
    NS_IGNORE_UNUSED(lparam);
  }
};

// can't use a 'using' here, because that can't be forward declared
class NS_CORE_DLL nsWindow : public nsWindowWin
{
};

#endif
