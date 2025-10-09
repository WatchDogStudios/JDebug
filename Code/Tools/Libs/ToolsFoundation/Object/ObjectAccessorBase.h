#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class nsDocumentObject;

class NS_TOOLSFOUNDATION_DLL nsObjectAccessorBase : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsObjectAccessorBase, nsReflectedClass);

public:
  virtual ~nsObjectAccessorBase();
  const nsDocumentObjectManager* GetObjectManager() const;

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(nsStringView sDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(nsStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const nsDocumentObject* GetObject(const nsUuid& object) = 0;
  virtual nsStatus GetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant& out_value, nsVariant index = nsVariant()) = 0;
  virtual nsStatus SetValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index = nsVariant()) = 0;
  virtual nsStatus InsertValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& newValue, nsVariant index = nsVariant()) = 0;
  virtual nsStatus RemoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant()) = 0;
  virtual nsStatus MoveValue(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsVariant& oldIndex, const nsVariant& newIndex) = 0;
  virtual nsStatus GetCount(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsInt32& out_iCount) = 0;

  virtual nsStatus AddObject(const nsDocumentObject* pParent, const nsAbstractProperty* pParentProp, const nsVariant& index, const nsRTTI* pType,
    nsUuid& inout_objectGuid) = 0;
  virtual nsStatus RemoveObject(const nsDocumentObject* pObject) = 0;
  virtual nsStatus MoveObject(const nsDocumentObject* pObject, const nsDocumentObject* pNewParent, const nsAbstractProperty* pParentProp, const nsVariant& index) = 0;

  virtual nsStatus GetKeys(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_keys) = 0;
  virtual nsStatus GetValues(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsDynamicArray<nsVariant>& out_values) = 0;

  /// \brief If this accessor is a proxy accessor, transform the input parameters into those of the source accessor. The default implementation does nothing and returns this.
  /// Usually this only needs to be implemented on nsObjectProxyAccessor derived accessors that modify the type, property, view etc of an object.
  /// @param ref_pObject In: proxy object, out: source object.
  /// @param ref_pType In: proxy type, out: source type.
  /// @param ref_pProp In: proxy property, out: source property.
  /// @param ref_indices In: proxy indices, out: source indices. While most of the time this will be one index, e.g. an array or map index. In case of variants that can store containers in containers this can be a chain of indices into a variant hierarchy.
  /// @return Returns the source accessor.
  virtual nsObjectAccessorBase* ResolveProxy(const nsDocumentObject*& ref_pObject, const nsRTTI*& ref_pType, const nsAbstractProperty*& ref_pProp, nsDynamicArray<nsVariant>& ref_indices) { return this; }
  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  nsStatus GetValueByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant& out_value, nsVariant index = nsVariant());
  nsStatus SetValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& newValue, nsVariant index = nsVariant());
  nsStatus InsertValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& newValue, nsVariant index = nsVariant());
  nsStatus RemoveValueByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index = nsVariant());
  nsStatus MoveValueByName(const nsDocumentObject* pObject, nsStringView sProp, const nsVariant& oldIndex, const nsVariant& newIndex);
  nsStatus GetCountByName(const nsDocumentObject* pObject, nsStringView sProp, nsInt32& out_iCount);

  nsStatus AddObjectByName(const nsDocumentObject* pParent, nsStringView sParentProp, const nsVariant& index, const nsRTTI* pType, nsUuid& inout_objectGuid);
  nsStatus MoveObjectByName(const nsDocumentObject* pObject, const nsDocumentObject* pNewParent, nsStringView sParentProp, const nsVariant& index);

  nsStatus GetKeysByName(const nsDocumentObject* pObject, nsStringView sProp, nsDynamicArray<nsVariant>& out_keys);
  nsStatus GetValuesByName(const nsDocumentObject* pObject, nsStringView sProp, nsDynamicArray<nsVariant>& out_values);
  const nsDocumentObject* GetChildObjectByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index);

  nsStatus ClearByName(const nsDocumentObject* pObject, nsStringView sProp);

  const nsAbstractProperty* FindPropertyByName(const nsDocumentObject* pObject, nsStringView sProp);

  template <typename T>
  T Get(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index = nsVariant());
  template <typename T>
  T GetByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index = nsVariant());
  nsInt32 GetCount(const nsDocumentObject* pObject, const nsAbstractProperty* pProp);
  nsInt32 GetCountByName(const nsDocumentObject* pObject, nsStringView sProp);

  ///@}

protected:
  nsObjectAccessorBase(const nsDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const nsDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const nsDocumentObjectPropertyEvent& e);

protected:
  const nsDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
