#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>

class nsQtContainerWindow;
namespace ads
{
  class CDockManager;
}

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class NS_GUIFOUNDATION_DLL nsQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  nsQtApplicationPanel(ads::CDockManager* pDockManager, const char* szPanelName);
  ~nsQtApplicationPanel();

  void EnsureVisible();

  static const nsDynamicArray<nsQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const nsToolsProjectEvent& e);
  virtual bool event(QEvent* event) override;

private:
  friend class nsQtContainerWindow;

  static nsDynamicArray<nsQtApplicationPanel*> s_AllApplicationPanels;

  nsQtContainerWindow* m_pContainerWindow = nullptr;
};

NS_DECLARE_REFLECTABLE_TYPE(NS_GUIFOUNDATION_DLL, nsQtApplicationPanel);
