#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

nsCommandLineOptionInt opt_PID("_MiniDumpTool", "-PID", "Process ID of the application for which to create a crash dump.", 0);

nsCommandLineOptionPath opt_DumpFile("_MiniDumpTool", "-f", "Path to the crash dump file to write.", "");

class nsMiniDumpTool : public nsApplication
{
  nsUInt32 m_uiProcessID = 0;
  nsStringBuilder m_sDumpFile;

public:
  using SUPER = nsApplication;

  nsMiniDumpTool()
    : nsApplication("MiniDumpTool")
  {
  }

  nsResult ParseArguments()
  {
    nsCommandLineUtils* cmd = nsCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");

    m_sDumpFile = opt_DumpFile.GetOptionValue(nsCommandLineOption::LogMode::Always);
    m_sDumpFile.MakeCleanPath();

    if (m_uiProcessID == 0)
    {
      nsLog::Error("Missing '-PID' argument");
      return NS_FAILURE;
    }

    return NS_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    nsFileSystem::AddDataDirectory("", "App", ":", nsDataDirUsage::AllowWrites).IgnoreResult();

    nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void Run() override
  {
    {
      nsStringBuilder cmdHelp;
      if (nsCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, nsCommandLineOption::LogAvailableModes::IfHelpRequested, "_MiniDumpTool"))
      {
        nsLog::Print(cmdHelp);
        RequestApplicationQuit();
        return;
      }
    }

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      RequestApplicationQuit();
      return;
    }

    nsMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID).IgnoreResult();
    RequestApplicationQuit();
  }
};

NS_APPLICATION_ENTRY_POINT(nsMiniDumpTool);
