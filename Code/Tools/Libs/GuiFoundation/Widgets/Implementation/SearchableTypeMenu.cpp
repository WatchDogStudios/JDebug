#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

bool nsQtTypeMenu::s_bShowInDevelopmentFeatures = false;
nsDynamicArray<nsString>* nsQtTypeMenu::s_pRecentList = nullptr;

struct TypeComparer
{
  NS_FORCE_INLINE bool Less(const nsRTTI* a, const nsRTTI* b) const
  {
    const nsCategoryAttribute* pCatA = a->GetAttributeByType<nsCategoryAttribute>();
    const nsCategoryAttribute* pCatB = b->GetAttributeByType<nsCategoryAttribute>();
    if (pCatA != nullptr && pCatB == nullptr)
    {
      return true;
    }
    else if (pCatA == nullptr && pCatB != nullptr)
    {
      return false;
    }
    else if (pCatA != nullptr && pCatB != nullptr)
    {
      nsInt32 iRes = nsStringUtils::Compare(pCatA->GetCategory(), pCatB->GetCategory());
      if (iRes != 0)
      {
        return iRes < 0;
      }
    }

    return a->GetTypeName().Compare(b->GetTypeName()) < 0;
  }
};

nsString nsQtTypeMenu::s_sLastMenuSearch;

QMenu* nsQtTypeMenu::CreateCategoryMenu(const char* szCategory, nsMap<nsString, QMenu*>& existingMenus)
{
  if (nsStringUtils::IsNullOrEmpty(szCategory))
    return m_pMenu;


  auto it = existingMenus.Find(szCategory);
  if (it.IsValid())
    return it.Value();

  nsStringBuilder sPath = szCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QMenu* pParentMenu = m_pMenu;

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath, existingMenus);
  }

  sPath = szCategory;
  sPath = sPath.GetFileName();

  QMenu* pNewMenu = pParentMenu->addMenu(nsMakeQString(nsTranslate(sPath)));
  existingMenus[szCategory] = pNewMenu;

  return pNewMenu;
}

void nsQtTypeMenu::OnMenuAction()
{
  const nsRTTI* pRtti = static_cast<const nsRTTI*>(sender()->property("type").value<void*>());

  OnMenuAction(pRtti);
}

void nsQtTypeMenu::OnMenuAction(const nsRTTI* pRtti)
{
  m_pLastSelectedType = pRtti;

  if (s_pRecentList && !s_pRecentList->Contains(pRtti->GetTypeName()))
  {
    if (s_pRecentList->GetCount() > 32)
    {
      s_pRecentList->RemoveAtAndCopy(0);
    }

    s_pRecentList->PushBack(pRtti->GetTypeName());
  }

  Q_EMIT TypeSelected(nsMakeQString(pRtti->GetTypeName()));
}

void nsQtTypeMenu::FillMenu(QMenu* pMenu, const nsRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu)
{
  m_pMenu = pMenu;

  m_SupportedTypes.Clear();
  m_SupportedTypes.Insert(pBaseType);

  if (bDerivedTypes)
  {
    nsReflectionUtils::GatherTypesDerivedFromClass(pBaseType, m_SupportedTypes);
  }

  // Make category-sorted array of types and skip all abstract, hidden or in development types
  nsDynamicArray<const nsRTTI*> supportedTypes;
  for (const nsRTTI* pRtti : m_SupportedTypes)
  {
    if (pRtti->GetTypeFlags().IsAnySet(nsTypeFlags::Abstract))
      continue;

    if (pRtti->GetAttributeByType<nsHiddenAttribute>() != nullptr)
      continue;

    if (!s_bShowInDevelopmentFeatures && pRtti->GetAttributeByType<nsInDevelopmentAttribute>() != nullptr)
      continue;

    supportedTypes.PushBack(pRtti);
  }
  supportedTypes.Sort(TypeComparer());

  if (!bSimpleMenu && supportedTypes.GetCount() > 10)
  {
    // only show a searchable menu when it makes some sense
    // also deactivating entries to prevent duplicates is currently not supported by the searchable menu
    m_pSearchableMenu = new nsQtSearchableMenu(m_pMenu);
  }

  nsStringBuilder sIconName;
  nsStringBuilder sCategory = "";

  nsMap<nsString, QMenu*> existingMenus;

  if (m_pSearchableMenu == nullptr)
  {
    // first round: create all sub menus
    for (const nsRTTI* pRtti : supportedTypes)
    {
      // Determine current menu
      const nsCategoryAttribute* pCatA = pRtti->GetAttributeByType<nsCategoryAttribute>();

      if (pCatA)
      {
        CreateCategoryMenu(pCatA->GetCategory(), existingMenus);
      }
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    // add recently used sub-menu
    if (s_pRecentList)
    {
      nsStringBuilder sInternalPath, sDisplayName;

      nsInt32 iToAdd = 8;

      for (auto& sTypeName : *s_pRecentList)
      {
        const nsRTTI* pRtti = nsRTTI::FindTypeByName(sTypeName);

        if (pRtti == nullptr)
          continue;

        if (!pRtti->IsDerivedFrom(pBaseType))
          continue;

        sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

        sInternalPath.Set(" *** RECENT ***/", pRtti->GetTypeName());

        sDisplayName = nsTranslate(pRtti->GetTypeName());

        const nsCategoryAttribute* pCatA = pRtti->GetAttributeByType<nsCategoryAttribute>();
        const nsColorAttribute* pColA = pRtti->GetAttributeByType<nsColorAttribute>();

        nsColor iconColor = nsColor::MakeZero();

        if (pColA)
        {
          iconColor = pColA->GetColor();
        }
        else if (pCatA && iconColor == nsColor::MakeZero())
        {
          iconColor = nsColorScheme::GetCategoryColor(pCatA->GetCategory(), nsColorScheme::CategoryColorUsage::MenuEntryIcon);
        }

        const QIcon actionIcon = nsQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

        m_pSearchableMenu->AddItem(sDisplayName, sInternalPath, QVariant::fromValue((void*)pRtti), actionIcon);

        if (--iToAdd <= 0)
          break;
      }
    }
  }

  nsStringBuilder tmp;

  // second round: create the actions
  for (const nsRTTI* pRtti : supportedTypes)
  {
    sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

    // Determine current menu
    const nsCategoryAttribute* pCatA = pRtti->GetAttributeByType<nsCategoryAttribute>();
    const nsInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<nsInDevelopmentAttribute>();
    const nsColorAttribute* pColA = pRtti->GetAttributeByType<nsColorAttribute>();

    nsColor iconColor = nsColor::MakeZero();

    if (pColA)
    {
      iconColor = pColA->GetColor();
    }
    else if (pCatA && iconColor == nsColor::MakeZero())
    {
      iconColor = nsColorScheme::GetCategoryColor(pCatA->GetCategory(), nsColorScheme::CategoryColorUsage::MenuEntryIcon);
    }

    const QIcon actionIcon = nsQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);


    if (m_pSearchableMenu != nullptr)
    {
      nsStringBuilder sFullPath;
      sFullPath = pCatA ? pCatA->GetCategory() : "";
      sFullPath.AppendPath(pRtti->GetTypeName());

      nsStringBuilder sDisplayName = nsTranslate(pRtti->GetTypeName());
      if (pInDev)
      {
        sDisplayName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      m_pSearchableMenu->AddItem(sDisplayName, sFullPath, QVariant::fromValue((void*)pRtti), actionIcon);
    }
    else
    {
      QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

      nsStringBuilder fullName = nsTranslate(pRtti->GetTypeName());

      if (pInDev)
      {
        fullName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      // Add type action to current menu
      QAction* pAction = new QAction(fullName.GetData(), m_pMenu);
      pAction->setProperty("type", QVariant::fromValue((void*)pRtti));
      NS_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

      pAction->setIcon(actionIcon);

      pCat->addAction(pAction);
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    connect(m_pSearchableMenu, &nsQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
      {
        const nsRTTI* pRtti = static_cast<const nsRTTI*>(variant.value<void*>());

        OnMenuAction(pRtti);

        m_pMenu->close();
        //
      });

    connect(m_pSearchableMenu, &nsQtSearchableMenu::SearchTextChanged, m_pMenu,
      [this](const QString& sText)
      { nsQtTypeMenu::s_sLastMenuSearch = sText.toUtf8().data(); });

    m_pMenu->addAction(m_pSearchableMenu);

    // important to do this last to make sure the search bar gets focus
    m_pSearchableMenu->Finalize(nsQtTypeMenu::s_sLastMenuSearch.GetData());
  }
}
