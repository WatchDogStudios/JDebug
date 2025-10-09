#pragma once

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class nsDocumentObject;

/// \brief Accessor for a sub-tree on an nsVariant property.
/// The tools foundation code uses an nsDocumentObject, one of its nsAbstractProperty and an optional nsVariant index to reference to properties. Any deeper hierarchies must be built from additional objects. This principle prevents the GUI to reference anything inside an nsVariant that stores an VariantArray or VariantDictionary as nsVariant is a pure value type and cannot store additional objects on the tool side. To work around this, this class creates a view one level deeper into an nsVariant. This is done by calling SetSubItems which for each object in the map moves the view into the sub-tree referenced by the given value of the map.
class NS_TOOLSFOUNDATION_DLL nsVariantSubAccessor : public nsObjectProxyAccessor
{
  NS_ADD_DYNAMIC_REFLECTION(nsVariantSubAccessor, nsObjectProxyAccessor);

public:
  /// \brief Constructor
  /// \param pSource The original accessor that is going to be proxied. By chaining this class an nsVariant can be explored deeper and deeper.
  /// \param pProp The nsVariant property that is going to be proxied. Only this property is allowed to be accessed by the accessor functions.
  nsVariantSubAccessor(nsObjectAccessorBase* pSource, const nsAbstractProperty* pProp);
  /// \brief Sets the sub-tree indices for the selected objects.
  /// \param subItemMap Object to index map. Note that as this is in the ToolsFoundation it cannot use the nsPropertySelection class.
  void SetSubItems(const nsMap<const nsDocumentObject*, nsVariant>& subItemMap);
  /// \brief Returns the property this accessor wraps.
  const nsAbstractProperty* GetRootProperty() const { return m_pProp; }
  /// \brief How many level deep the view is inside the property.
  nsInt32 GetDepth() const;
  /// Builds a path up the hierarchy of wrapped nsVariantSubAccessor objects to determine the path to the current sub-tree of the nsVariant.
  /// \param pObject The object for which the path should be computed
  /// \param out_path An array of indices that has to be followed from the root of the nsVariant to each the current sub-tree view.
  /// \return Returns NS_FAILURE if pObject is not known.
  nsResult GetPath(const nsDocumentObject* pObject, nsDynamicArray<nsVariant>& out_path) const;

  virtual nsStatus GetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant& out_value, nsVariant index = nsVariant()) override;
  virtual nsStatus SetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index = nsVariant()) override;
  virtual nsStatus InsertValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index = nsVariant()) override;
  virtual nsStatus RemoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant()) override;
  virtual nsStatus MoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& oldIndex, const nsVariant& newIndex) override;
  virtual nsStatus GetCount(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsInt32& out_iCount) override;
  virtual nsStatus GetKeys(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_keys) override;
  virtual nsStatus GetValues(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_values) override;

  virtual nsObjectAccessorBase* ResolveProxy(const nsDocumentObject*& ref_pObject, const nsRTTI*& ref_pType, const nsAbstractProperty*& ref_pProp, nsDynamicArray<nsVariant>& ref_indices) override;

private:
  nsStatus GetSubValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant& out_value);
  nsStatus SetSubValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsDelegate<nsStatus(nsVariant& subValue)>& func);

private:
  const nsAbstractProperty* m_pProp = nullptr;
  nsMap<const nsDocumentObject*, nsVariant> m_SubItemMap;
};
