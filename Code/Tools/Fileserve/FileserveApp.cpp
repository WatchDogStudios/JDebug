#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>

void nsFileserverApp::AfterCoreSystemsStartup()
{
  nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  nsFileSystem::AddDataDirectory("", "App", ":", nsDataDirUsage::AllowWrites).IgnoreResult();

  NS_DEFAULT_NEW(nsFileserver);

  nsFileserver::GetSingleton()->m_Events.AddEventHandler(nsMakeDelegate(&nsFileserverApp::FileserverEventHandler, this));

#ifndef NS_USE_QT
  nsFileserver::GetSingleton()->m_Events.AddEventHandler(nsMakeDelegate(&nsFileserverApp::FileserverEventHandlerConsole, this));
  nsFileserver::GetSingleton()->StartServer();
#endif

  // Load all available shader compiler plugins
  {
    nsFileSystemIterator it;
    nsStringBuilder sShaderCompilerSearch(nsOSFile::GetApplicationDirectory(), "/nsShaderCompiler*");
    sShaderCompilerSearch.MakeCleanPath();

    for (it.StartSearch(sShaderCompilerSearch, nsFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
    {
      nsStringBuilder sName = it.GetStats().m_sName;

      if (sName.HasExtension("DLL"))
      {
        sName.RemoveFileExtension();
        nsPlugin::LoadPlugin(sName, nsPluginLoadFlags::PluginIsOptional).IgnoreResult();
      }
    }
  }

  nsFileserver::GetSingleton()->SetCustomMessageHandler('SHDR', nsMakeDelegate(&nsFileserverApp::ShaderMessageHandler, this));

  // TODO: CommandLine Option
  m_CloseAppTimeout = nsTime::MakeFromSeconds(nsCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_close_timeout", 0));
  m_TimeTillClosing = nsTime::MakeFromSeconds(nsCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_wait_timeout", 0));

  if (m_TimeTillClosing.GetSeconds() > 0)
  {
    m_TimeTillClosing += nsTime::Now();
  }
}

void nsFileserverApp::BeforeCoreSystemsShutdown()
{
  nsFileserver::GetSingleton()->StopServer();

#ifndef NS_USE_QT
  nsFileserver::GetSingleton()->m_Events.RemoveEventHandler(nsMakeDelegate(&nsFileserverApp::FileserverEventHandlerConsole, this));
#endif

  nsFileserver::GetSingleton()->m_Events.RemoveEventHandler(nsMakeDelegate(&nsFileserverApp::FileserverEventHandler, this));

  nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

void nsFileserverApp::Run()
{
  // if there are no more connections, and we have a timeout to close when no connections are left, we return Quit
  if (m_uiConnections == 0 && m_TimeTillClosing > nsTime::MakeFromSeconds(0) && nsTime::Now() > m_TimeTillClosing)
  {
    RequestApplicationQuit();
    return;
  }

  if (nsFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(10));
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(1));
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }
}
