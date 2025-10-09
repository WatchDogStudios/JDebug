#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

////////////////////////////////////////////////////////////////////////
// nsReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

nsReflectedTypeStorageAccessor::nsReflectedTypeStorageAccessor(const nsRTTI* pRtti, nsDocumentObject* pOwner)
  : nsIReflectedTypeAccessor(pRtti, pOwner)
{
  const nsRTTI* pType = pRtti;
  NS_ASSERT_DEV(pType != nullptr, "Trying to construct an nsReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = nsReflectedTypeStorageManager::AddStorageAccessor(this);
  NS_ASSERT_DEV(m_pMapping != nullptr, "The type for this nsReflectedTypeStorageAccessor is unknown to the nsReflectedTypeStorageManager!");

  auto& indexTable = m_pMapping->m_PathToStorageInfoTable;
  const nsUInt32 uiProperties = indexTable.GetCount();
  // To prevent re-allocs due to new properties being added we reserve 20% more space.
  m_Data.Reserve(uiProperties + uiProperties / 20);
  m_Data.SetCount(uiProperties);

  // Fill data storage with default values for the given types.
  for (auto it = indexTable.GetIterator(); it.IsValid(); ++it)
  {
    const auto& storageInfo = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

nsReflectedTypeStorageAccessor::~nsReflectedTypeStorageAccessor()
{
  nsReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const nsVariant nsReflectedTypeStorageAccessor::GetValue(nsStringView sProperty, nsVariant index, nsStatus* pRes) const
{
  const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = nsStatus(nsFmt("Property '{0}' not found in type '{1}'", sProperty, GetType()->GetTypeName()));
    return nsVariant();
  }

  if (pRes)
    *pRes = nsStatus(NS_SUCCESS);
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Member:
        if (index.IsValid())
        {
          if (pRes)
          {
            *pRes = nsStatus(nsFmt("Property '{0}' is a member property but an index of '{1}' is given", sProperty, index));
          }
          return nsVariant();
        }
        return m_Data[storageInfo->m_uiIndex];
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      case nsPropertyCategory::Map:
      {
        return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetValue(index, pRes);
      }
      break;
      default:
        break;
    }
  }
  return nsVariant();
}

bool nsReflectedTypeStorageAccessor::SetValue(nsStringView sProperty, const nsVariant& value, nsVariant index)
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;
    NS_ASSERT_DEV(pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>() || value.IsValid(), "");

    if (storageInfo->m_Type == nsVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = nsReflectionUtils::IsValueType(pProp);
    const nsVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(nsPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(nsPropertyFlags::Class) && !isValueType) ? nsVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Member:
      {
        if (index.IsValid())
          return false;

        if (value.IsA<nsString>() && pProp->GetFlags().IsAnySet(nsPropertyFlags::IsEnum | nsPropertyFlags::Bitflags))
        {
          nsInt64 iValue;
          nsReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<nsString>(), iValue);
          m_Data[storageInfo->m_uiIndex] = nsVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>())
        {
          m_Data[storageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(storageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
          return true;
        }
      }
      break;
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>())
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      case nsPropertyCategory::Map:
      {
        if (index.IsA<nsString>())
        {
          if (pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>())
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType), index).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

nsInt32 nsReflectedTypeStorageAccessor::GetCount(nsStringView sProperty) const
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == nsVariant::Type::Invalid)
      return false;

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      case nsPropertyCategory::Map:
        return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetCount();
      default:
        break;
    }
  }
  return -1;
}

bool nsReflectedTypeStorageAccessor::GetKeys(nsStringView sProperty, nsDynamicArray<nsVariant>& out_keys) const
{
  out_keys.Clear();

  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == nsVariant::Type::Invalid)
      return false;

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      case nsPropertyCategory::Map:
      {
        return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).GetKeys(out_keys).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}
bool nsReflectedTypeStorageAccessor::InsertValue(nsStringView sProperty, nsVariant index, const nsVariant& value)
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == nsVariant::Type::Invalid)
      return false;

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    if (storageInfo->m_Type == nsVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = nsReflectionUtils::IsValueType(pProp);
    const nsVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(nsPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(nsPropertyFlags::Class) && !isValueType) ? nsVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>())
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      case nsPropertyCategory::Map:
      {
        if (index.IsA<nsString>())
        {
          if (pProp->GetSpecificType() == nsGetStaticRTTI<nsVariant>())
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType)).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool nsReflectedTypeStorageAccessor::RemoveValue(nsStringView sProperty, nsVariant index)
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == nsVariant::Type::Invalid)
      return false;

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      case nsPropertyCategory::Map:
      {
        return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).RemoveValue(index).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool nsReflectedTypeStorageAccessor::MoveValue(nsStringView sProperty, nsVariant oldIndex, nsVariant newIndex)
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == nsVariant::Type::Invalid)
      return false;

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      case nsPropertyCategory::Map:
      {
        return nsVariantStorageAccessor(sProperty, m_Data[storageInfo->m_uiIndex]).MoveValue(oldIndex, newIndex).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

nsVariant nsReflectedTypeStorageAccessor::GetPropertyChildIndex(nsStringView sProperty, const nsVariant& value) const
{
  const nsReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    //    if (storageInfo->m_Type == nsVariant::Type::Invalid)
    //      return nsVariant();

    const nsAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return nsVariant();

    const bool isValueType = nsReflectionUtils::IsValueType(pProp);
    const nsVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(nsPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(nsPropertyFlags::Class) && !isValueType) ? nsVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case nsPropertyCategory::Array:
      case nsPropertyCategory::Set:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const nsVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<nsVariantArray>();
          for (nsUInt32 i = 0; i < values.GetCount(); i++)
          {
            if (values[i] == value)
              return nsVariant((nsUInt32)i);
          }
        }
      }
      break;
      case nsPropertyCategory::Map:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const nsVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<nsVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return nsVariant(it.Key());
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return nsVariant();
}
