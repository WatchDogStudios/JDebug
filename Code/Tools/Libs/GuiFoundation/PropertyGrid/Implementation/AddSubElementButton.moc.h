#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;

/// \brief Used by container widgets to add new elements to the container.
class NS_GUIFOUNDATION_DLL nsQtAddSubElementButton : public nsQtPropertyWidget
{
  Q_OBJECT

public:
  /// Constructor
  /// \param containerCategory The type of container. Only Map, Set and Array are supported.
  nsQtAddSubElementButton(nsEnum<nsPropertyCategory> containerCategory, nsStringView sButtonText);

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void onMenuAboutToShow();
  void on_Button_clicked();
  void OnTypeSelected(QString sTypeName);

private:
  virtual void OnInit() override;
  void OnAction(const nsRTTI* pRtti);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  nsQtTypeMenu m_TypeMenu;

  nsEnum<nsPropertyCategory> m_ContainerCategory;
  bool m_bNoMoreElementsAllowed = false;
  QMenu* m_pMenu = nullptr;
  nsUInt32 m_uiMaxElements = 0; // 0 means unlimited
  bool m_bPreventDuplicates = false;
};
