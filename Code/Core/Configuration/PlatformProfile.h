#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class nsChunkStreamWriter;
class nsChunkStreamReader;

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class NS_CORE_DLL nsProfileConfigData : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsProfileConfigData, nsReflectedClass);

public:
  nsProfileConfigData();
  ~nsProfileConfigData();

  virtual void SaveRuntimeData(nsChunkStreamWriter& inout_stream) const;
  virtual void LoadRuntimeData(nsChunkStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Stores platform-specific configuration data for asset processing and runtime settings.
///
/// A platform profile contains multiple configuration objects (nsProfileConfigData) that store
/// settings for different aspects like asset transforms, rendering options, etc. Each profile
/// targets a specific platform and maintains a modification counter for change tracking.
class NS_CORE_DLL nsPlatformProfile final : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsPlatformProfile, nsReflectedClass);

public:
  nsPlatformProfile();
  ~nsPlatformProfile();

  void SetConfigName(nsStringView sName) { m_sName = sName; }
  nsStringView GetConfigName() const { return m_sName; }

  void SetTargetPlatform(nsStringView sPlatform) { m_sTargetPlatform = sPlatform; }
  nsStringView GetTargetPlatform() const { return m_sTargetPlatform; }

  void Clear();
  void AddMissingConfigs();

  template <typename TYPE>
  const TYPE* GetTypeConfig() const
  {
    return static_cast<const TYPE*>(GetTypeConfig(nsGetStaticRTTI<TYPE>()));
  }

  template <typename TYPE>
  TYPE* GetTypeConfig()
  {
    return static_cast<TYPE*>(GetTypeConfig(nsGetStaticRTTI<TYPE>()));
  }

  const nsProfileConfigData* GetTypeConfig(const nsRTTI* pRtti) const;
  nsProfileConfigData* GetTypeConfig(const nsRTTI* pRtti);

  nsResult SaveForRuntime(nsStringView sFile) const;
  nsResult LoadForRuntime(nsStringView sFile);

  /// \brief Returns a number indicating when the profile counter changed last. By storing and comparing this value, other code can update their state if necessary.
  nsUInt32 GetLastModificationCounter() const { return m_uiLastModificationCounter; }


private:
  nsUInt32 m_uiLastModificationCounter = 0;
  nsString m_sName;
  nsString m_sTargetPlatform = "Windows";
  nsDynamicArray<nsProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
