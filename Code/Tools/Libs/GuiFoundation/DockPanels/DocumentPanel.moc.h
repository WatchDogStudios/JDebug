#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ads/DockWidget.h>

class nsDocument;

class NS_GUIFOUNDATION_DLL nsQtDocumentPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  nsQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, nsDocument* pDocument);
  ~nsQtDocumentPanel();

  virtual bool event(QEvent* pEvent) override;

private:
  nsDocument* m_pDocument = nullptr;
};
