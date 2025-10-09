#pragma once

#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct nsStatus;

/// \brief Helper class to modify an nsVariant as if it was a container.
/// GetValue and SetValue are valid for all variant types.
/// The remaining accessor functions require an VariantArray or VariantDictionary type.
class NS_TOOLSFOUNDATION_DLL nsVariantStorageAccessor
{
public:
  nsVariantStorageAccessor(nsStringView sProperty, nsVariant& value);
  nsVariantStorageAccessor(nsStringView sProperty, const nsVariant& value);

  nsVariant GetValue(nsVariant index = nsVariant(), nsStatus* pRes = nullptr) const;
  nsStatus SetValue(const nsVariant& value, nsVariant index = nsVariant());

  nsInt32 GetCount() const;
  nsStatus GetKeys(nsDynamicArray<nsVariant>& out_keys) const;
  nsStatus InsertValue(const nsVariant& index, const nsVariant& value);
  nsStatus RemoveValue(const nsVariant& index);
  nsStatus MoveValue(const nsVariant& oldIndex, const nsVariant& newIndex);

private:
  nsStringView m_sProperty;
  nsVariant& m_Value;
};
