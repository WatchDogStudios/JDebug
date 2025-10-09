
// can't use a 'using' here, because that can't be forward declared
class NS_CORE_DLL nsWindowAndroid : public nsWindowPlatformShared
{
public:
  ~nsWindowAndroid();

  virtual nsResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual nsResult Resize(const nsSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual void OnResize(const nsSizeU32& newWindowSize) override;
  virtual nsWindowHandle GetNativeWindowHandle() const override;
};

// can't use a 'using' here, because that can't be forward declared
class NS_CORE_DLL nsWindow : public nsWindowAndroid
{
};
