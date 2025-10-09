#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

nsVariantStorageAccessor::nsVariantStorageAccessor(nsStringView sProperty, nsVariant& value)
  : m_sProperty(sProperty)
  , m_Value(value)
{
}

nsVariantStorageAccessor::nsVariantStorageAccessor(nsStringView sProperty, const nsVariant& value)
  : m_sProperty(sProperty)
  , m_Value(const_cast<nsVariant&>(value))
{
}

nsVariant nsVariantStorageAccessor::GetValue(nsVariant index, nsStatus* pRes) const
{
  if (!index.IsValid())
    return m_Value;

  if (index.IsNumber())
  {
    if (!m_Value.IsA<nsVariantArray>())
    {
      if (pRes)
        *pRes = nsStatus(nsFmt("Index '{0}' for property '{1}' is invalid as the property is not an array.", index, m_sProperty));
      return nsVariant();
    }
    const nsVariantArray& values = m_Value.Get<nsVariantArray>();
    nsUInt32 uiIndex = index.ConvertTo<nsUInt32>();
    if (uiIndex < values.GetCount())
    {
      return values[uiIndex];
    }
  }
  else if (index.IsA<nsString>())
  {
    if (!m_Value.IsA<nsVariantDictionary>())
    {
      if (pRes)
        *pRes = nsStatus(nsFmt("Index '{0}' for property '{1}' is invalid as the property is not a dictionary.", index, m_sProperty));
      return nsVariant();
    }
    const nsVariantDictionary& values = m_Value.Get<nsVariantDictionary>();
    const nsString& sIndex = index.Get<nsString>();
    if (const nsVariant* pValue = values.GetValue(sIndex))
    {
      return *pValue;
    }
  }

  if (pRes)
    *pRes = nsStatus(nsFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, m_sProperty));
  return nsVariant();
}

nsStatus nsVariantStorageAccessor::SetValue(const nsVariant& value, nsVariant index)
{
  if (!index.IsValid())
  {
    m_Value = value;
    return NS_SUCCESS;
  }

  if (index.IsNumber() && m_Value.IsA<nsVariantArray>())
  {
    nsVariantArray& values = m_Value.GetWritable<nsVariantArray>();
    nsUInt32 uiIndex = index.ConvertTo<nsUInt32>();
    if (uiIndex >= values.GetCount())
    {
      return nsStatus(nsFmt("Index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));
    }
    values[uiIndex] = value;
    return NS_SUCCESS;
  }
  else if (index.IsA<nsString>() && m_Value.IsA<nsVariantDictionary>())
  {
    nsVariantDictionary& values = m_Value.GetWritable<nsVariantDictionary>();
    const nsString& sIndex = index.Get<nsString>();
    if (!values.Contains(sIndex))
    {
      return nsStatus(nsFmt("Index '{0}' for property '{1}' is out of bounds.", sIndex, m_sProperty));
    }
    values[sIndex] = value;
    return NS_SUCCESS;
  }
  return nsStatus(nsFmt("Index '{0}' for property '{1}' is invalid.", index, m_sProperty));
}

nsInt32 nsVariantStorageAccessor::GetCount() const
{
  if (m_Value.IsA<nsVariantArray>())
    return m_Value.Get<nsVariantArray>().GetCount();
  else if (m_Value.IsA<nsVariantDictionary>())
    return m_Value.Get<nsVariantDictionary>().GetCount();
  return 0;
}

nsStatus nsVariantStorageAccessor::GetKeys(nsDynamicArray<nsVariant>& out_keys) const
{
  if (m_Value.IsA<nsVariantArray>())
  {
    const nsVariantArray& values = m_Value.Get<nsVariantArray>();
    out_keys.Reserve(values.GetCount());
    for (nsUInt32 i = 0; i < values.GetCount(); ++i)
    {
      out_keys.PushBack(i);
    }
    return NS_SUCCESS;
  }
  else if (m_Value.IsA<nsVariantDictionary>())
  {
    const nsVariantDictionary& values = m_Value.Get<nsVariantDictionary>();
    out_keys.Reserve(values.GetCount());
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(nsVariant(it.Key()));
    }
    return NS_SUCCESS;
  }
  return nsStatus(nsFmt("Property '{0}' is not a container.", m_sProperty));
}

nsStatus nsVariantStorageAccessor::InsertValue(const nsVariant& index, const nsVariant& value)
{
  if (index.IsNumber() && m_Value.IsA<nsVariantArray>())
  {
    nsVariantArray& values = m_Value.GetWritable<nsVariantArray>();
    nsInt32 iIndex = index.ConvertTo<nsInt32>();
    const nsInt32 iCount = (nsInt32)values.GetCount();
    if (iIndex == -1)
    {
      iIndex = iCount;
    }
    if (iIndex > iCount)
      return nsStatus(nsFmt("InsertValue: index '{0}' for property '{1}' is out of bounds.", iIndex, m_sProperty));

    values.InsertAt(iIndex, value);
    return NS_SUCCESS;
  }
  else if (index.IsA<nsString>() && m_Value.IsA<nsVariantDictionary>())
  {
    nsVariantDictionary& values = m_Value.GetWritable<nsVariantDictionary>();
    const nsString& sIndex = index.Get<nsString>();
    if (values.Contains(index.Get<nsString>()))
      return nsStatus(nsFmt("InsertValue: index '{0}' for property '{1}' already exists.", sIndex, m_sProperty));

    values.Insert(sIndex, value);
    return NS_SUCCESS;
  }
  return nsStatus(nsFmt("InsertValue: Property '{0}' is not a container or index {1} is invalid.", m_sProperty, index));
}

nsStatus nsVariantStorageAccessor::RemoveValue(const nsVariant& index)
{
  if (index.IsNumber() && m_Value.IsA<nsVariantArray>())
  {
    nsVariantArray& values = m_Value.GetWritable<nsVariantArray>();
    const nsUInt32 uiIndex = index.ConvertTo<nsUInt32>();
    if (uiIndex > values.GetCount())
      return nsStatus(nsFmt("RemoveValue: index '{0}' for property '{1}' is out of bounds.", uiIndex, m_sProperty));

    values.RemoveAtAndCopy(uiIndex);
    return NS_SUCCESS;
  }
  else if (index.IsA<nsString>() && m_Value.IsA<nsVariantDictionary>())
  {
    nsVariantDictionary& values = m_Value.GetWritable<nsVariantDictionary>();
    const nsString& sIndex = index.Get<nsString>();
    if (!values.Contains(index.Get<nsString>()))
      return nsStatus(nsFmt("RemoveValue: index '{0}' for property '{1}' does not exists.", sIndex, m_sProperty));

    values.Remove(sIndex);
    return NS_SUCCESS;
  }
  return nsStatus(nsFmt("RemoveValue: Property '{0}' is not a container or index '{1}' is invalid.", m_sProperty, index));
}

nsStatus nsVariantStorageAccessor::MoveValue(const nsVariant& oldIndex, const nsVariant& newIndex)
{
  if (m_Value.IsA<nsVariantArray>() && oldIndex.IsNumber() && newIndex.IsNumber())
  {
    nsVariantArray& values = m_Value.GetWritable<nsVariantArray>();
    nsUInt32 uiOldIndex = oldIndex.ConvertTo<nsUInt32>();
    nsUInt32 uiNewIndex = newIndex.ConvertTo<nsUInt32>();
    if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
    {
      nsVariant value = values[uiOldIndex];
      values.RemoveAtAndCopy(uiOldIndex);
      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }
      values.InsertAt(uiNewIndex, value);
      return NS_SUCCESS;
    }
    else
    {
      return nsStatus(nsFmt("MoveValue: index '{0}' or '{1}' for property '{2}' is out of bounds.", uiOldIndex, uiNewIndex, m_sProperty));
    }
  }
  else if (m_Value.IsA<nsVariantDictionary>() && oldIndex.IsA<nsString>() && newIndex.IsA<nsString>())
  {
    nsVariantDictionary& values = m_Value.GetWritable<nsVariantDictionary>();
    const nsString& sOldIndex = oldIndex.Get<nsString>();
    const nsString& sNewIndex = newIndex.Get<nsString>();

    if (!values.Contains(sOldIndex))
      return nsStatus(nsFmt("MoveValue: old index '{0}' for property '{2}' does not exist.", sOldIndex, m_sProperty));
    else if (values.Contains(sNewIndex))
      return nsStatus(nsFmt("MoveValue: new index '{0}' for property '{2}' already exists.", sNewIndex, m_sProperty));

    values.Insert(sNewIndex, values[sOldIndex]);
    values.Remove(sOldIndex);
    return NS_SUCCESS;
  }
  return nsStatus(nsFmt("MoveValue: Property '{0}' is not a container or index '{1}' or '{2}' is invalid.", m_sProperty, oldIndex, newIndex));
}
