#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/Strings/HashedString.h>
#include <Utilities/UtilitiesDLL.h>

using nsConfigFileResourceHandle = nsTypedResourceHandle<class nsConfigFileResource>;

/// \brief This resource loads config files containing key/value pairs
///
/// The config files usually use the file extension '.nsConfig'.
///
/// The file format looks like this:
///
/// To declare a key/value pair for the first time, write its type, name and value:
///   int i = 1
///   float f = 2.3
///   bool b = false
///   string s = "hello"
///
/// To set a variable to a different value than before, it has to be marked with 'override':
///
///   override i = 4
///
/// The format supports C preprocessor features like #include, #define, #ifdef, etc.
/// This can be used to build hierarchical config files:
///
///   #include "BaseConfig.nsConfig"
///   override int SomeValue = 7
///
/// It can also be used to define 'enum types':
///
///   #define SmallValue 3
///   #define BigValue 5
///   int MyValue = BigValue
///
/// Since resources can be reloaded at runtime, config resources are a convenient way to define game parameters
/// that you may want to tweak at any time.
/// Using C preprocessor logic (#define, #if, #else, etc) you can quickly select between different configuration sets.
///
/// Once loaded, accessing the data is very efficient.
class NS_UTILITIES_DLL nsConfigFileResource : public nsResource
{
  NS_ADD_DYNAMIC_REFLECTION(nsConfigFileResource, nsResource);

  NS_RESOURCE_DECLARE_COMMON_CODE(nsConfigFileResource);

public:
  nsConfigFileResource();
  ~nsConfigFileResource();

  /// \brief Returns the 'int' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  nsInt32 GetInt(nsTempHashedString sName) const;

  /// \brief Returns the 'float' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  float GetFloat(nsTempHashedString sName) const;

  /// \brief Returns the 'bool' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  bool GetBool(nsTempHashedString sName) const;

  /// \brief Returns the 'string' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  nsStringView GetString(nsTempHashedString sName) const;

  /// \brief Returns the 'int' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  nsInt32 GetInt(nsTempHashedString sName, nsInt32 iFallback) const;

  /// \brief Returns the 'float' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  float GetFloat(nsTempHashedString sName, float fFallback) const;

  /// \brief Returns the 'bool' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  bool GetBool(nsTempHashedString sName, bool bFallback) const;

  /// \brief Returns the 'string' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  nsStringView GetString(nsTempHashedString sName, nsStringView sFallback) const;

protected:
  virtual nsResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual nsResourceLoadDesc UpdateContent(nsStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class nsConfigFileResourceLoader;

  nsHashTable<nsHashedString, nsInt32> m_IntData;
  nsHashTable<nsHashedString, float> m_FloatData;
  nsHashTable<nsHashedString, nsString> m_StringData;
  nsHashTable<nsHashedString, bool> m_BoolData;

  nsDependencyFile m_RequiredFiles;
};


class NS_UTILITIES_DLL nsConfigFileResourceLoader : public nsResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    nsDefaultMemoryStreamStorage m_Storage;
    nsMemoryStreamReader m_Reader;
    nsDependencyFile m_RequiredFiles;

    nsResult PrePropFileLocator(nsStringView sCurAbsoluteFile, nsStringView sIncludeFile, nsPreprocessor::IncludeType incType, nsStringBuilder& out_sAbsoluteFilePath);
  };

  virtual nsResourceLoadData OpenDataStream(const nsResource* pResource) override;
  virtual void CloseDataStream(const nsResource* pResource, const nsResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const nsResource* pResource) const override;
};
