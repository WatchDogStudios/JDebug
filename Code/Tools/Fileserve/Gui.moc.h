#pragma once

#include <Fileserve/Fileserve.h>

#ifdef NS_USE_QT

#  include <QMainWindow>

class nsApplication;
class nsQtFileserveWidget;

class nsQtFileserveMainWnd : public QMainWindow
{
  Q_OBJECT
public:
  nsQtFileserveMainWnd(nsApplication* pApp, QWidget* pParent = nullptr);

private Q_SLOTS:
  void UpdateNetworkSlot();
  void OnServerStarted(const QString& ip, nsUInt16 uiPort);
  void OnServerStopped();

private:
  nsApplication* m_pApp;
  nsQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(nsApplication* pApp);

#endif
