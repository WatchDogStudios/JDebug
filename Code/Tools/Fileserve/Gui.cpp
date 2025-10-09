#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef NS_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>

void CreateFileserveMainWindow(nsApplication* pApp)
{
  nsQtFileserveMainWnd* pMainWnd = new nsQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

nsQtFileserveMainWnd::nsQtFileserveMainWnd(nsApplication* pApp, QWidget* pParent)
  : QMainWindow(pParent)
  , m_pApp(pApp)
{
  OnServerStopped();

  m_pFileserveWidget = new nsQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  resize(700, 650);

  connect(m_pFileserveWidget, &nsQtFileserveWidget::ServerStarted, this, &nsQtFileserveMainWnd::OnServerStarted);
  connect(m_pFileserveWidget, &nsQtFileserveWidget::ServerStopped, this, &nsQtFileserveMainWnd::OnServerStopped);

  show();

  QTimer::singleShot(0, this, &nsQtFileserveMainWnd::UpdateNetworkSlot);

  setWindowIcon(m_pFileserveWidget->windowIcon());
}


void nsQtFileserveMainWnd::UpdateNetworkSlot()
{
  m_pApp->Run();

  if (m_pApp->ShouldApplicationQuit())
  {
    close();
  }
  else
  {
    QTimer::singleShot(0, this, &nsQtFileserveMainWnd::UpdateNetworkSlot);
  }
}

void nsQtFileserveMainWnd::OnServerStarted(const QString& ip, nsUInt16 uiPort)
{
  QString title = QString("nsFileserve (Port %1)").arg(uiPort);

  setWindowTitle(title);
}

void nsQtFileserveMainWnd::OnServerStopped()
{
  setWindowTitle("nsFileserve (not running)");
}

#endif
