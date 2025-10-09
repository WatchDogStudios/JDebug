#include <Core/CorePCH.h>

#include <Core/GameState/GameStateWindow.h>

nsGameStateWindow::nsGameStateWindow(const nsWindowCreationDesc& windowdesc, nsDelegate<void()> onClickClose)
  : m_OnClickClose(onClickClose)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  InitializeWindow().IgnoreResult();
}

nsGameStateWindow::~nsGameStateWindow()
{
  DestroyWindow();
}


void nsGameStateWindow::ResetOnClickClose(nsDelegate<void()> onClickClose)
{
  m_OnClickClose = onClickClose;
}

void nsGameStateWindow::OnClickClose()
{
  if (m_OnClickClose.IsValid())
  {
    m_OnClickClose();
  }
}

void nsGameStateWindow::OnResize(const nsSizeU32& newWindowSize)
{
  nsLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}
