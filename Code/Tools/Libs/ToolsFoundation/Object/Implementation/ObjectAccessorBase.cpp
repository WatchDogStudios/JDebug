#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsObjectAccessorBase, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void nsObjectAccessorBase::StartTransaction(nsStringView sDisplayString) {}


void nsObjectAccessorBase::CancelTransaction() {}


void nsObjectAccessorBase::FinishTransaction() {}


void nsObjectAccessorBase::BeginTemporaryCommands(nsStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void nsObjectAccessorBase::CancelTemporaryCommands() {}


void nsObjectAccessorBase::FinishTemporaryCommands() {}


nsStatus nsObjectAccessorBase::GetValueByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant& out_value, nsVariant index /*= nsVariant()*/)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


nsStatus nsObjectAccessorBase::SetValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& newValue, nsVariant index /*= nsVariant()*/)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


nsStatus nsObjectAccessorBase::InsertValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& newValue, nsVariant index /*= nsVariant()*/)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


nsStatus nsObjectAccessorBase::RemoveValueByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index /*= nsVariant()*/)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


nsStatus nsObjectAccessorBase::MoveValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& oldIndex, const nsVariant& newIndex)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


nsStatus nsObjectAccessorBase::GetCountByName(const nsDocumentObject* pObject, nsStringView sProp, nsInt32& out_iCount)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


nsStatus nsObjectAccessorBase::AddObjectByName(const nsDocumentObject* pParent, nsStringView sParentProp, const nsVariant& index, const nsRTTI* pType, nsUuid& inout_objectGuid)
{
  const nsAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

nsStatus nsObjectAccessorBase::MoveObjectByName(const nsDocumentObject* pObject, const nsDocumentObject* pNewParent, nsStringView sParentProp, const nsVariant& index)
{
  const nsAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


nsStatus nsObjectAccessorBase::GetKeysByName(const nsDocumentObject* pObject, nsStringView sProp, nsDynamicArray<nsVariant>& out_keys)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


nsStatus nsObjectAccessorBase::GetValuesByName(const nsDocumentObject* pObject, nsStringView sProp, nsDynamicArray<nsVariant>& out_values)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const nsDocumentObject* nsObjectAccessorBase::GetChildObjectByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index)
{
  nsVariant value;
  if (GetValueByName(pObject, sProp, value, index).Succeeded() && value.IsA<nsUuid>())
  {
    return GetObject(value.Get<nsUuid>());
  }
  return nullptr;
}

nsStatus nsObjectAccessorBase::ClearByName(const nsDocumentObject* pObject, nsStringView sProp)
{
  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return nsStatus(nsFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));

  nsHybridArray<nsVariant, 8> keys;
  nsStatus res = GetKeys(pObject, pProp, keys);
  if (res.Failed())
    return res;

  for (nsInt32 i = keys.GetCount() - 1; i >= 0; --i)
  {
    res = RemoveValue(pObject, pProp, keys[i]);
    if (res.Failed())
      return res;
  }
  return nsStatus(NS_SUCCESS);
}

const nsAbstractProperty* nsObjectAccessorBase::FindPropertyByName(const nsDocumentObject* pObject, nsStringView sProp)
{
  return pObject->GetType()->FindPropertyByName(sProp);
}

nsObjectAccessorBase::nsObjectAccessorBase(const nsDocumentObjectManager* pManager)
  : m_pConstManager(pManager)
{
}

nsObjectAccessorBase::~nsObjectAccessorBase() = default;

const nsDocumentObjectManager* nsObjectAccessorBase::GetObjectManager() const
{
  return m_pConstManager;
}

void nsObjectAccessorBase::FireDocumentObjectStructureEvent(const nsDocumentObjectStructureEvent& e)
{
  m_pConstManager->m_StructureEvents.Broadcast(e);
}

void nsObjectAccessorBase::FireDocumentObjectPropertyEvent(const nsDocumentObjectPropertyEvent& e)
{
  m_pConstManager->m_PropertyEvents.Broadcast(e);
}
