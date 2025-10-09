#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Platform/PlatformDesc.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

nsCommandLineOptionBool opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
nsCommandLineOptionInt opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", nsTelemetry::s_uiPort);
nsCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "Default");

nsString nsGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

nsString nsGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void nsGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void nsGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  if (opt_Profile.IsOptionSpecified())
  {
    m_PlatformProfile.SetConfigName(opt_Profile.GetOptionValue(nsCommandLineOption::LogMode::AlwaysIfSpecified));
  }
  else
  {
    m_PlatformProfile.SetConfigName(nsPlatformDesc::GetThisPlatformDesc().GetName());

    const nsStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".nsProfile");

    if (!nsFileSystem::ExistsFile(sRuntimeProfileFile))
    {
      nsLog::Info("Platform profile '{}' doesn't exist, switching to 'Default'", m_PlatformProfile.GetConfigName());

      m_PlatformProfile.SetConfigName("Default");
    }
  }

  m_PlatformProfile.AddMissingConfigs();
}

void nsGameApplicationBase::BaseInit_ConfigureLogging()
{
#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
  nsGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  nsGlobalLog::RemoveLogWriter(m_LogToVsID);

  if (!opt_DisableConsoleOutput.GetOptionValue(nsCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void nsGameApplicationBase::Init_ConfigureTelemetry()
{
#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
  nsTelemetry::s_uiPort = static_cast<nsUInt16>(opt_TelemetryPort.GetOptionValue(nsCommandLineOption::LogMode::AlwaysIfSpecified));
  nsTelemetry::SetServerName(GetApplicationName());
  nsTelemetry::CreateServer();
#endif
}

void nsGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  nsFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void nsGameApplicationBase::Init_ConfigureAssetManagement() {}

void nsGameApplicationBase::Init_LoadRequiredPlugins()
{
  nsPlugin::InitializeStaticallyLinkedPlugins();

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  nsPlugin::LoadPlugin("XBoxControllerPlugin", nsPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void nsGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see nsFileSystem::ResolveSpecialDirectory

  const nsStringBuilder sUserDataPath(">user/", GetApplicationName());

  nsFileSystem::CreateDirectoryStructure(sUserDataPath).AssertSuccess();

  nsString writableBinRoot = ">appdir/";
  nsString shaderCacheRoot = ">sdk/Output/";

#if NS_DISABLED(NS_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on mobile platforms
  writableBinRoot = sUserDataPath;
#endif

  nsFileSystem::CreateDirectoryStructure(shaderCacheRoot).IgnoreResult();

  // for absolute paths, read-only
  nsFileSystem::AddDataDirectory("", "GameApplicationBase", ":", nsDataDirUsage::ReadOnly).AssertSuccess();

  // ":bin/" : writing to the binary directory
  nsFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", nsDataDirUsage::AllowWrites).AssertSuccess();

  // ":shadercache/" for reading and writing shader files
#if NS_DISABLED(NS_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  nsFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", nsDataDirUsage::ReadOnly).AssertSuccess();
#else
  nsFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", nsDataDirUsage::AllowWrites).AssertSuccess();
#endif

  // ":appdata/" for reading and writing app user data
  nsFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", nsDataDirUsage::AllowWrites).AssertSuccess();

  // ":base/" for reading the core engine files
  nsFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", nsDataDirUsage::ReadOnly).IgnoreResult();

  // ":project/" for reading the project specific files
  nsFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", nsDataDirUsage::ReadOnly).IgnoreResult();

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    nsStringBuilder dir;
    nsFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", dir).IgnoreResult();
    if (dir.IsAbsolutePath() && nsOSFile::ExistsDirectory(dir))
    {
      nsFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", nsDataDirUsage::ReadOnly).IgnoreResult();
    }
  }

  {
    nsApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (nsUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const nsString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void nsGameApplicationBase::Init_LoadWorldModuleConfig()
{
  nsWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
}

void nsGameApplicationBase::Init_LoadProjectPlugins()
{
  nsApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.Apply();
}

void nsGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const nsStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".nsProfile");
  m_PlatformProfile.AddMissingConfigs();

  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void nsGameApplicationBase::Init_ConfigureTags()
{
  NS_LOG_BLOCK("Reading Tags", "Tags.ddl");

  nsStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

  nsFileReader file;
  if (file.Open(sFile).Failed())
  {
    nsLog::Dev("'{}' does not exist", sFile);
    return;
  }

  nsStringBuilder tmp;

  nsOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    nsLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const nsOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const nsOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const nsOpenDdlReaderElement* pName = pTags->FindChildOfType(nsOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      nsLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    nsTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void nsGameApplicationBase::Init_ConfigureCVars()
{
  nsCVar::SetStorageFolder(":appdata/CVars");
  nsCVar::LoadCVars();
}

void nsGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  nsResourceManager::SetAutoFreeUnused(nsTime::MakeFromMicroseconds(100), nsTime::MakeFromSeconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void nsGameApplicationBase::Deinit_UnloadPlugins()
{
  nsPlugin::UnloadAllPlugins();
}

void nsGameApplicationBase::Deinit_ShutdownLogging()
{
#if NS_DISABLED(NS_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  nsGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  nsGlobalLog::RemoveLogWriter(m_LogToVsID);
#endif
}
