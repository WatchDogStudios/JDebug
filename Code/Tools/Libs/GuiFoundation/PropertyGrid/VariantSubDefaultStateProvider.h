#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class nsVariantSubAccessor;

// \brief Default value provider for nsVariantSubAccessor.
class NS_GUIFOUNDATION_DLL nsVariantSubDefaultStateProvider : public nsDefaultStateProvider
{
public:
  static nsSharedPtr<nsDefaultStateProvider> CreateProvider(nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp);

  nsVariantSubDefaultStateProvider(nsVariantSubAccessor* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp);

  virtual nsInt32 GetRootDepth() const override;
  virtual nsColorGammaUB GetBackgroundColor() const override;
  virtual nsString GetStateProviderName() const override { return "Variant"; }

  virtual nsVariant GetDefaultValue(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant()) override;
  virtual nsStatus CreateRevertContainerDiff(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDeque<nsAbstractGraphDiffOperation>& out_diff) override;
  virtual bool IsDefaultValue(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant()) override;
  virtual nsStatus RevertProperty(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant()) override;

private:
  nsResult GetDefaultValueInternal(SuperArray superPtr, nsObjectAccessorBase* pAccessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index, nsVariant& out_DefaultValue);

private:
  nsVariantSubAccessor* m_pAccessor = nullptr;
  const nsDocumentObject* m_pObject = nullptr;
  const nsAbstractProperty* m_pProp = nullptr;
  nsObjectAccessorBase* m_pRootAccessor = nullptr;
};
