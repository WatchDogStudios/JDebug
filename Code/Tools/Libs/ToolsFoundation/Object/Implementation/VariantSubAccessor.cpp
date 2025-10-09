#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsVariantSubAccessor, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

nsVariantSubAccessor::nsVariantSubAccessor(nsObjectAccessorBase* pSource, const nsAbstractProperty* pProp)
  : nsObjectProxyAccessor(pSource)
  , m_pProp(pProp)
{
}

void nsVariantSubAccessor::SetSubItems(const nsMap<const nsDocumentObject*, nsVariant>& subItemMap)
{
  m_SubItemMap = subItemMap;
}

nsInt32 nsVariantSubAccessor::GetDepth() const
{
  if (auto variantSubAccessor = nsDynamicCast<nsVariantSubAccessor*>(GetSourceAccessor()))
  {
    return variantSubAccessor->GetDepth() + 1;
  }
  return 1;
}

nsResult nsVariantSubAccessor::GetPath(const nsDocumentObject* pObject, nsDynamicArray<nsVariant>& out_path) const
{
  out_path.Clear();
  if (auto variantSubAccessor = nsDynamicCast<nsVariantSubAccessor*>(GetSourceAccessor()))
  {
    NS_SUCCEED_OR_RETURN(variantSubAccessor->GetPath(pObject, out_path));
  }
  nsVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return NS_FAILURE;

  out_path.PushBack(subItem);
  return NS_SUCCESS;
}

nsStatus nsVariantSubAccessor::GetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant& out_value, nsVariant index)
{
  NS_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  nsStatus result(NS_SUCCESS);
  out_value = nsVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

nsStatus nsVariantSubAccessor::SetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index)
{
  return SetSubValue(pObject, pProp, [&](nsVariant& subValue) -> nsStatus
    { return nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

nsStatus nsVariantSubAccessor::InsertValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index)
{
  return SetSubValue(pObject, pProp, [&](nsVariant& subValue) -> nsStatus
    { return nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

nsStatus nsVariantSubAccessor::RemoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index)
{
  return SetSubValue(pObject, pProp, [&](nsVariant& subValue) -> nsStatus
    { return nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

nsStatus nsVariantSubAccessor::MoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& oldIndex, const nsVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](nsVariant& subValue) -> nsStatus
    { return nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

nsStatus nsVariantSubAccessor::GetCount(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsInt32& out_iCount)
{
  nsVariant subValue;
  NS_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return NS_SUCCESS;
}

nsStatus nsVariantSubAccessor::GetKeys(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_keys)
{
  nsVariant subValue;
  NS_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return nsVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

nsStatus nsVariantSubAccessor::GetValues(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_values)
{
  nsVariant subValue;
  NS_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  nsHybridArray<nsVariant, 16> keys;
  nsVariantStorageAccessor accessor(pProp->GetPropertyName(), subValue);
  NS_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const nsVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return NS_SUCCESS;
}

nsObjectAccessorBase* nsVariantSubAccessor::ResolveProxy(const nsDocumentObject*& ref_pObject, const nsRTTI*& ref_pType, const nsAbstractProperty*& ref_pProp, nsDynamicArray<nsVariant>& ref_indices)
{
  nsVariant subItem;
  if (m_SubItemMap.TryGetValue(ref_pObject, subItem))
  {
    ref_indices.InsertAt(0, subItem);
  }

  return m_pSource->ResolveProxy(ref_pObject, ref_pType, ref_pProp, ref_indices);
}

nsStatus nsVariantSubAccessor::GetSubValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant& out_value)
{
  nsStatus result = nsObjectProxyAccessor::GetValue(pObject, pProp, out_value);
  if (result.Failed())
    return result;

  nsVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return nsStatus(nsFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  out_value = nsVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(subItem, &result);
  return result;
}

nsStatus nsVariantSubAccessor::SetSubValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsDelegate<nsStatus(nsVariant&)>& func)
{
  NS_ASSERT_DEBUG(m_pProp == pProp, "nsVariantSubAccessor should only be used to access a single variant property");
  nsVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return nsStatus(nsFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  nsVariant currentValue;
  NS_SUCCEED_OR_RETURN(nsObjectProxyAccessor::GetValue(pObject, pProp, currentValue, subItem));
  NS_SUCCEED_OR_RETURN(func(currentValue));
  return nsObjectProxyAccessor::SetValue(pObject, pProp, currentValue, subItem);
}
