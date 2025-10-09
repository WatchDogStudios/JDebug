#pragma once

#include <Foundation/Containers/Set.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QObject>

class nsQtSearchableMenu;
class nsRTTI;
class QMenu;

class NS_GUIFOUNDATION_DLL nsQtTypeMenu : public QObject
{
  Q_OBJECT

public:
  void FillMenu(QMenu* pMenu, const nsRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu);

  static nsDynamicArray<nsString>* s_pRecentList;
  static bool s_bShowInDevelopmentFeatures;

  const nsRTTI* m_pLastSelectedType = nullptr;

Q_SIGNALS:
  void TypeSelected(QString sTypeName);

protected Q_SLOTS:
  void OnMenuAction();

private:
  QMenu* CreateCategoryMenu(const char* szCategory, nsMap<nsString, QMenu*>& existingMenus);
  void OnMenuAction(const nsRTTI* pRtti);

  QMenu* m_pMenu = nullptr;
  nsSet<const nsRTTI*> m_SupportedTypes;
  nsQtSearchableMenu* m_pSearchableMenu = nullptr;

  static nsString s_sLastMenuSearch;
};
