#include <Utilities/UtilitiesPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Utilities/Resources/ConfigFileResource.h>

static nsConfigFileResourceLoader s_ConfigFileResourceLoader;

// clang-format off
NS_BEGIN_SUBSYSTEM_DECLARATION(Utilties, ConfigFileResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    nsResourceManager::SetResourceTypeLoader<nsConfigFileResource>(&s_ConfigFileResourceLoader);

    auto hFallback = nsResourceManager::LoadResource<nsConfigFileResource>("Empty.nsConfig");
    nsResourceManager::SetResourceTypeMissingFallback<nsConfigFileResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    nsResourceManager::SetResourceTypeMissingFallback<nsConfigFileResource>(nsConfigFileResourceHandle());
    nsResourceManager::SetResourceTypeLoader<nsConfigFileResource>(nullptr);
    nsConfigFileResource::CleanupDynamicPluginReferences();
  }

  NS_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsConfigFileResource, 1, nsRTTIDefaultAllocator<nsConfigFileResource>)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

NS_RESOURCE_IMPLEMENT_COMMON_CODE(nsConfigFileResource);

nsConfigFileResource::nsConfigFileResource()
  : nsResource(nsResource::DoUpdate::OnAnyThread, 0)
{
}

nsConfigFileResource::~nsConfigFileResource() = default;

nsInt32 nsConfigFileResource::GetInt(nsTempHashedString sName, nsInt32 iFallback) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return iFallback;
}

nsInt32 nsConfigFileResource::GetInt(nsTempHashedString sName) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  nsStringView name = "<unknown>"_nssv;
  sName.LookupStringHash(name).IgnoreResult();
  nsLog::Error("{}: 'int' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

float nsConfigFileResource::GetFloat(nsTempHashedString sName, float fFallback) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

float nsConfigFileResource::GetFloat(nsTempHashedString sName) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  nsStringView name = "<unknown>"_nssv;
  sName.LookupStringHash(name).IgnoreResult();
  nsLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

bool nsConfigFileResource::GetBool(nsTempHashedString sName, bool bFallback) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return bFallback;
}

bool nsConfigFileResource::GetBool(nsTempHashedString sName) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  nsStringView name = "<unknown>"_nssv;
  sName.LookupStringHash(name).IgnoreResult();
  nsLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return false;
}

nsStringView nsConfigFileResource::GetString(nsTempHashedString sName, nsStringView sFallback) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return sFallback;
}

nsStringView nsConfigFileResource::GetString(nsTempHashedString sName) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  nsStringView name = "<unknown>"_nssv;
  sName.LookupStringHash(name).IgnoreResult();
  nsLog::Error("{}: 'string' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return "";
}

nsResourceLoadDesc nsConfigFileResource::UnloadData(Unload WhatToUnload)
{
  NS_IGNORE_UNUSED(WhatToUnload);

  m_IntData.Clear();
  m_FloatData.Clear();
  m_StringData.Clear();
  m_BoolData.Clear();

  nsResourceLoadDesc d;
  d.m_State = nsResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

nsResourceLoadDesc nsConfigFileResource::UpdateContent(nsStreamReader* Stream)
{
  nsResourceLoadDesc d;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  d.m_State = nsResourceState::Loaded;

  if (Stream == nullptr)
  {
    d.m_State = nsResourceState::LoadedResourceMissing;
    return d;
  }

  m_RequiredFiles.ReadDependencyFile(*Stream).IgnoreResult();
  Stream->ReadHashTable(m_IntData).IgnoreResult();
  Stream->ReadHashTable(m_FloatData).IgnoreResult();
  Stream->ReadHashTable(m_StringData).IgnoreResult();
  Stream->ReadHashTable(m_BoolData).IgnoreResult();

  return d;
}

void nsConfigFileResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = m_IntData.GetHeapMemoryUsage() + m_FloatData.GetHeapMemoryUsage() + m_StringData.GetHeapMemoryUsage() + m_BoolData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

nsResult nsConfigFileResourceLoader::LoadedData::PrePropFileLocator(nsStringView sCurAbsoluteFile, nsStringView sIncludeFile, nsPreprocessor::IncludeType incType, nsStringBuilder& out_sAbsoluteFilePath)
{
  nsResult res = nsPreprocessor::DefaultFileLocator(sCurAbsoluteFile, sIncludeFile, incType, out_sAbsoluteFilePath);

  m_RequiredFiles.AddFileDependency(out_sAbsoluteFilePath);

  return res;
}

nsResourceLoadData nsConfigFileResourceLoader::OpenDataStream(const nsResource* pResource)
{
  NS_PROFILE_SCOPE("ReadResourceFile");
  NS_LOG_BLOCK("Load Config Resource", pResource->GetResourceID());

  nsStringBuilder sConfig;

  nsMap<nsString, nsInt32> intData;
  nsMap<nsString, float> floatData;
  nsMap<nsString, nsString> stringData;
  nsMap<nsString, bool> boolData;

  LoadedData* pData = NS_DEFAULT_NEW(LoadedData);
  pData->m_Reader.SetStorage(&pData->m_Storage);

  nsPreprocessor preprop;

  // used to gather all the transitive file dependencies
  preprop.SetFileLocatorFunction(nsMakeDelegate(&nsConfigFileResourceLoader::LoadedData::PrePropFileLocator, pData));

  if (pResource->GetResourceID() == "Empty.nsConfig")
  {
    // do nothing
  }
  else if (preprop.Process(pResource->GetResourceID(), sConfig, false, true, false).Succeeded())
  {
    sConfig.ReplaceAll("\r", "");
    sConfig.ReplaceAll("\n", ";");

    nsHybridArray<nsStringView, 32> lines;
    sConfig.Split(false, lines, ";");

    nsStringBuilder key, value, line;

    for (nsStringView tmp : lines)
    {
      line = tmp;
      line.Trim(" \t");

      if (line.IsEmpty())
        continue;

      const char* szAssign = line.FindSubString("=");

      if (szAssign == nullptr)
      {
        nsLog::Error("Invalid line in config file: '{}'", tmp);
      }
      else
      {
        value = szAssign + 1;
        value.Trim(" ");

        line.SetSubString_FromTo(line.GetData(), szAssign);
        line.ReplaceAll("\t", " ");
        line.ReplaceAll("  ", " ");
        line.Trim(" ");

        const bool bOverride = line.TrimWordStart("override ");
        line.Trim(" ");

        if (line.StartsWith("int "))
        {
          key.SetSubString_FromTo(line.GetData() + 4, szAssign);
          key.Trim(" ");

          if (bOverride && !intData.Contains(key))
            nsLog::Error("Config 'int' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && intData.Contains(key))
            nsLog::Error("Config 'int' key '{}' is not marked override, but exist already. Use 'override int' instead.", key);

          nsInt32 val;
          if (nsConversionUtils::StringToInt(value, val).Succeeded())
          {
            intData[key] = val;
          }
          else
          {
            nsLog::Error("Failed to parse 'int' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("float "))
        {
          key.SetSubString_FromTo(line.GetData() + 6, szAssign);
          key.Trim(" ");

          if (bOverride && !floatData.Contains(key))
            nsLog::Error("Config 'float' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && floatData.Contains(key))
            nsLog::Error("Config 'float' key '{}' is not marked override, but exist already. Use 'override float' instead.", key);

          double val;
          if (nsConversionUtils::StringToFloat(value, val).Succeeded())
          {
            floatData[key] = (float)val;
          }
          else
          {
            nsLog::Error("Failed to parse 'float' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("bool "))
        {
          key.SetSubString_FromTo(line.GetData() + 5, szAssign);
          key.Trim(" ");

          if (bOverride && !boolData.Contains(key))
            nsLog::Error("Config 'bool' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && boolData.Contains(key))
            nsLog::Error("Config 'bool' key '{}' is not marked override, but exist already. Use 'override bool' instead.", key);

          bool val;
          if (nsConversionUtils::StringToBool(value, val).Succeeded())
          {
            boolData[key] = val;
          }
          else
          {
            nsLog::Error("Failed to parse 'bool' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("string "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, szAssign);
          key.Trim(" ");

          if (bOverride && !stringData.Contains(key))
            nsLog::Error("Config 'string' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && stringData.Contains(key))
            nsLog::Error("Config 'string' key '{}' is not marked override, but exist already. Use 'override string' instead.", key);

          if (!value.StartsWith("\"") || !value.EndsWith("\""))
          {
            nsLog::Error("Failed to parse 'string' in config file: '{}'", tmp);
          }
          else
          {
            value.Shrink(1, 1);
            stringData[key] = value;
          }
        }
        else
        {
          nsLog::Error("Invalid line in config file: '{}'", tmp);
        }
      }
    }
  }
  else
  {
    // empty stream
    return {};
  }

  nsResourceLoadData res;
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

#if NS_ENABLED(NS_SUPPORTS_FILE_STATS)
  nsFileStats stat;
  if (nsFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_sResourceDescription = stat.m_sName;
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }
#endif

  nsMemoryStreamWriter writer(&pData->m_Storage);

  pData->m_RequiredFiles.StoreCurrentTimeStamp();
  pData->m_RequiredFiles.WriteDependencyFile(writer).IgnoreResult();
  writer.WriteMap(intData).IgnoreResult();
  writer.WriteMap(floatData).IgnoreResult();
  writer.WriteMap(stringData).IgnoreResult();
  writer.WriteMap(boolData).IgnoreResult();

  return res;
}

void nsConfigFileResourceLoader::CloseDataStream(const nsResource* pResource, const nsResourceLoadData& loaderData)
{
  NS_IGNORE_UNUSED(pResource);

  LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);

  NS_DEFAULT_DELETE(pData);
}

bool nsConfigFileResourceLoader::IsResourceOutdated(const nsResource* pResource) const
{
  return static_cast<const nsConfigFileResource*>(pResource)->m_RequiredFiles.HasAnyFileChanged();
}


NS_STATICLINK_FILE(Utilities, Utilities_Resources_ConfigFileResource);
