#pragma once

#include <Core/System/Window.h>

/// A window class that expands on nsWindow with game-specific functionality.
///
/// Default window type used by nsGameState to create a game window.
/// Provides customizable close behavior through delegate callbacks.
class NS_CORE_DLL nsGameStateWindow : public nsWindow
{
public:
  nsGameStateWindow(const nsWindowCreationDesc& windowdesc, nsDelegate<void()> onClickClose = {});
  ~nsGameStateWindow();

  void ResetOnClickClose(nsDelegate<void()> onClickClose);

private:
  virtual void OnResize(const nsSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  nsDelegate<void()> m_OnClickClose;
};
