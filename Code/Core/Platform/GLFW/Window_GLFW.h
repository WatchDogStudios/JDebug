
class NS_CORE_DLL nsWindowGLFW : public nsWindowPlatformShared
{
public:
  ~nsWindowGLFW();

  virtual nsResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual nsResult Resize(const nsSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual void OnResize(const nsSizeU32& newWindowSize) override;
  virtual nsWindowHandle GetNativeWindowHandle() const override;

private:
  static void IconifyCallback(GLFWwindow* window, int iconified);
  static void SizeCallback(GLFWwindow* window, int width, int height);
  static void PositionCallback(GLFWwindow* window, int xpos, int ypos);
  static void CloseCallback(GLFWwindow* window);
  static void FocusCallback(GLFWwindow* window, int focused);
  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);
  static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

// can't use a 'using' here, because that can't be forward declared
class NS_CORE_DLL nsWindow : public nsWindowGLFW
{
};
