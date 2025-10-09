#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

nsStateMap::nsStateMap() = default;
nsStateMap::~nsStateMap() = default;


void nsStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void nsStateMap::StoreBool(const nsTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void nsStateMap::StoreInteger(const nsTempHashedString& sName, nsInt64 value)
{
  m_Integers[sName] = value;
}

void nsStateMap::StoreDouble(const nsTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void nsStateMap::StoreVec3(const nsTempHashedString& sName, const nsVec3& value)
{
  m_Vec3s[sName] = value;
}

void nsStateMap::StoreColor(const nsTempHashedString& sName, const nsColor& value)
{
  m_Colors[sName] = value;
}

void nsStateMap::StoreString(const nsTempHashedString& sName, const nsString& value)
{
  m_Strings[sName] = value;
}

void nsStateMap::RetrieveBool(const nsTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void nsStateMap::RetrieveInteger(const nsTempHashedString& sName, nsInt64& out_iValue, nsInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void nsStateMap::RetrieveDouble(const nsTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void nsStateMap::RetrieveVec3(const nsTempHashedString& sName, nsVec3& out_vValue, nsVec3 vDefaultValue /*= nsVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void nsStateMap::RetrieveColor(const nsTempHashedString& sName, nsColor& out_value, nsColor defaultValue /*= nsColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void nsStateMap::RetrieveString(const nsTempHashedString& sName, nsString& out_sValue, nsStringView sDefaultValue /*= {} */)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = sDefaultValue;
  }
}
