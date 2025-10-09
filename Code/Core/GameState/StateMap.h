#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// A simple registry that stores name/value pairs of types that are common to store game state.
///
/// Provides type-safe storage and retrieval of common data types used in game state management.
/// Values are stored by name and can be retrieved with optional default values.
class NS_CORE_DLL nsStateMap
{
public:
  nsStateMap();
  ~nsStateMap();

  /// void Load(nsStreamReader& stream);
  /// void Save(nsStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const nsTempHashedString& sName, bool value);
  void StoreInteger(const nsTempHashedString& sName, nsInt64 value);
  void StoreDouble(const nsTempHashedString& sName, double value);
  void StoreVec3(const nsTempHashedString& sName, const nsVec3& value);
  void StoreColor(const nsTempHashedString& sName, const nsColor& value);
  void StoreString(const nsTempHashedString& sName, const nsString& value);

  void RetrieveBool(const nsTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const nsTempHashedString& sName, nsInt64& out_iValue, nsInt64 iDefaultValue = 0);
  void RetrieveDouble(const nsTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const nsTempHashedString& sName, nsVec3& out_vValue, nsVec3 vDefaultValue = nsVec3(0));
  void RetrieveColor(const nsTempHashedString& sName, nsColor& out_value, nsColor defaultValue = nsColor::White);
  void RetrieveString(const nsTempHashedString& sName, nsString& out_sValue, nsStringView sDefaultValue = {});

private:
  nsHashTable<nsTempHashedString, bool> m_Bools;
  nsHashTable<nsTempHashedString, nsInt64> m_Integers;
  nsHashTable<nsTempHashedString, double> m_Doubles;
  nsHashTable<nsTempHashedString, nsVec3> m_Vec3s;
  nsHashTable<nsTempHashedString, nsColor> m_Colors;
  nsHashTable<nsTempHashedString, nsString> m_Strings;
};
