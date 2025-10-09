#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

nsQtDocumentPanel::nsQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, nsDocument* pDocument)
  : ads::CDockWidget(pDockManager, "nsQtDocumentPanel", pParent)
{
  m_pDocument = pDocument;

  setMinimumWidth(300);
  setMinimumHeight(200);

  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetMovable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFocusable, true);
}

nsQtDocumentPanel::~nsQtDocumentPanel() = default;

bool nsQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (nsQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }

  return CDockWidget::event(pEvent);
}
