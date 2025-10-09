#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_Prefabs, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new nsFunctionArgumentAttributes(1, new nsAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new nsFunctionArgumentAttributes(3, new nsDefaultValueAttribute(nsVariant(nsInvalidIndex))),
      new nsFunctionArgumentAttributes(4, new nsDefaultValueAttribute(true)),
      new nsFunctionArgumentAttributes(5, new nsDefaultValueAttribute(true))),

    NS_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "LocalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new nsFunctionArgumentAttributes(1, new nsAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new nsFunctionArgumentAttributes(4, new nsDefaultValueAttribute(nsVariant(nsInvalidIndex))),
      new nsFunctionArgumentAttributes(5, new nsDefaultValueAttribute(true)),
      new nsFunctionArgumentAttributes(6, new nsDefaultValueAttribute(true))),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("Prefabs"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on

void SpawnPrefabHelper(nsWorld& ref_world, nsStringView sPrefab, nsGameObjectHandle hParent, const nsTransform& transform, nsUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon, nsVariantArray& out_rootObjects)
{
  nsPrefabResourceHandle hPrefab = nsResourceManager::LoadResource<nsPrefabResource>(sPrefab);

  nsResourceLock<nsPrefabResource> pPrefab(hPrefab, nsResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != nsResourceAcquireResult::Final)
    return;

  nsHybridArray<nsGameObject*, 8> createdRootObjects;
  nsHybridArray<nsGameObject*, 8> createdChildObjects;

  nsPrefabInstantiationOptions opt;
  opt.m_hParent = hParent;
  opt.m_pCreatedRootObjectsOut = &createdRootObjects;
  opt.m_pCreatedChildObjectsOut = &createdChildObjects;

  pPrefab->InstantiatePrefab(ref_world, transform, opt);

  auto FixupObject = [&](nsGameObject* pObject)
  {
    if (uiUniqueID != nsInvalidIndex)
    {
      for (auto pComponent : pObject->GetComponents())
      {
        pComponent->SetUniqueID(uiUniqueID);
      }
    }

    if (bSetCreatedByPrefab)
      pObject->SetCreatedByPrefab();

    if (bSetHideShapeIcon)
      pObject->SetHideShapeIcon();
  };

  for (auto pObject : createdRootObjects)
  {
    FixupObject(pObject);
    out_rootObjects.PushBack(pObject->GetHandle());
  }

  for (auto pObject : createdChildObjects)
  {
    FixupObject(pObject);
  }
}

nsVariantArray nsScriptExtensionClass_Prefabs::SpawnPrefab(nsWorld* pWorld, nsStringView sPrefab, const nsTransform& globalTransform, nsUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  nsVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, nsGameObjectHandle(), globalTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}

nsVariantArray nsScriptExtensionClass_Prefabs::SpawnPrefabAsChild(nsWorld* pWorld, nsStringView sPrefab, nsGameObject* pParent, const nsTransform& localTransform, nsUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  nsVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, pParent != nullptr ? pParent->GetHandle() : nsGameObjectHandle(), localTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}


NS_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Prefabs);
