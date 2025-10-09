#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

nsSharedPtr<nsDefaultStateProvider> nsVariantSubDefaultStateProvider::CreateProvider(nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp)
{
  if (auto variantSubAccessor = nsDynamicCast<nsVariantSubAccessor*>(pAccessor))
  {
    if (variantSubAccessor->GetRootProperty() == pProp)
      return NS_DEFAULT_NEW(nsVariantSubDefaultStateProvider, variantSubAccessor, pObject, pProp);
  }
  return nullptr;
}

nsVariantSubDefaultStateProvider::nsVariantSubDefaultStateProvider(nsVariantSubAccessor* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp)
  : m_pAccessor(pAccessor)
  , m_pObject(pObject)
  , m_pProp(pProp)
{
  m_pRootAccessor = m_pAccessor->GetSourceAccessor();
  while (auto variantSubAccessor = nsDynamicCast<nsVariantSubAccessor*>(m_pRootAccessor))
  {
    m_pRootAccessor = variantSubAccessor->GetSourceAccessor();
  }
}

nsInt32 nsVariantSubDefaultStateProvider::GetRootDepth() const
{
  // As this default provider dives into the contents of a variable it has to always be executed first as all the other providers work on property granularity.
  return 1000;
}

nsColorGammaUB nsVariantSubDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return nsColorGammaUB(0, 0, 0, 0);
}

nsVariant nsVariantSubDefaultStateProvider::GetDefaultValue(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index)
{
  nsVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

nsStatus nsVariantSubDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDeque<nsAbstractGraphDiffOperation>& out_diff)
{
  NS_REPORT_FAILURE("Unreachable code");
  return nsStatus(NS_SUCCESS);
}

bool nsVariantSubDefaultStateProvider::IsDefaultValue(nsDefaultStateProvider::SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index)
{
  nsVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  nsVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

nsStatus nsVariantSubDefaultStateProvider::RevertProperty(nsDefaultStateProvider::SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index)
{
  nsVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return nsStatus(nsFmt("Failed to retrieve default value for variant sub tree."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

nsResult nsVariantSubDefaultStateProvider::GetDefaultValueInternal(nsDefaultStateProvider::SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index, nsVariant& out_DefaultValue)
{
  NS_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "nsVariantSubDefaultStateProvider is only valid on the object and variant property it was created on.");
  // As m_pAccessor is a view into an nsVariant we first need to take the same steps into the defaultValue retrieved from the root accessor to have the same view so we can compare the same subset of both nsVariants.
  out_DefaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), m_pRootAccessor, pObject, pProp);

  nsHybridArray<nsVariant, 4> path;
  NS_SUCCEED_OR_RETURN(m_pAccessor->GetPath(pObject, path));
  if (index.IsValid())
    path.PushBack(index);

  for (const nsVariant& step : path)
  {
    nsStatus res(NS_SUCCESS);
    out_DefaultValue = nsVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(step, &res);
    if (res.Failed())
      return NS_FAILURE;
  }
  return NS_SUCCESS;
}
