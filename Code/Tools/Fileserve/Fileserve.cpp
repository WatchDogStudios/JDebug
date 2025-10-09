#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#ifdef NS_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <QApplication>
#  include <QFileDialog>
#  include <QSettings>
#endif

#ifdef NS_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int iCmdShow)
{
#else
int main(int argc, const char** argv)
{
#endif
  nsFileserverApp* pApp = new nsFileserverApp();

#ifdef NS_USE_QT
  nsCommandLineUtils::GetGlobalInstance()->SetCommandLine();

  int argc = 0;
  char** argv = nullptr;
  QApplication* pQtApplication = new QApplication(argc, const_cast<char**>(argv));
  pQtApplication->setApplicationName("nsFileserve");
  pQtApplication->setOrganizationDomain("www.WDFramework.net");
  pQtApplication->setOrganizationName("WDFramework Project");
  pQtApplication->setApplicationVersion("1.0.0");

  nsRun_Startup(pApp).IgnoreResult();

  CreateFileserveMainWindow(pApp);
  pQtApplication->exec();
  nsRun_Shutdown(pApp);
#else
  pApp->SetCommandLineArguments((nsUInt32)argc, argv);
  nsRun(pApp);
#endif

  const int iReturnCode = pApp->GetReturnCode();
  if (iReturnCode != 0)
  {

    std::string text = pApp->TranslateReturnCode();
    if (!text.empty())
      printf("Return Code: '%s'\n", text.c_str());
  }

#ifdef NS_USE_QT
  delete pQtApplication;
#endif

  delete pApp;


  return iReturnCode;
}

nsResult nsFileserverApp::BeforeCoreSystemsStartup()
{
  nsStartup::AddApplicationTag("tool");
  nsStartup::AddApplicationTag("fileserve");

#ifdef NS_USE_QT
  if (!nsCommandLineUtils::GetGlobalInstance()->HasOption("-specialdirs"))
  {
    QString sLastFolder;

    {
      QSettings Settings;
      Settings.beginGroup(QLatin1String("Fileserve"));
      sLastFolder = Settings.value("LastProject", "").toString();
      Settings.endGroup();
    }

    QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Project Folder", sLastFolder);
    if (!folder.isEmpty())
    {
      QSettings Settings;
      Settings.beginGroup(QLatin1String("Fileserve"));
      Settings.setValue("LastProject", folder);
      Settings.endGroup();

      nsCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-specialdirs");
      nsCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("project");
      nsCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(folder.toUtf8().data());
    }
  }
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void nsFileserverApp::FileserverEventHandler(const nsFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case nsFileserverEvent::Type::ClientConnected:
    case nsFileserverEvent::Type::ClientReconnected:
      ++m_uiConnections;
      m_TimeTillClosing = nsTime::MakeZero();
      break;
    case nsFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = nsTime::Now() + m_CloseAppTimeout;
      }

      break;
    default:
      break;
  }
}

void nsFileserverApp::ShaderMessageHandler(nsFileserveClientContext& ref_ctxt, nsRemoteMessage& ref_msg, nsRemoteInterface& ref_clientChannel, nsDelegate<void(const char*)> logActivity)
{
  if (ref_msg.GetMessageID() == 'CMPL')
  {
    for (auto& dd : ref_ctxt.m_MountedDataDirs)
    {
      nsFileSystem::AddDataDirectory(dd.m_sPathOnServer, "FileServe", dd.m_sRootName, nsDataDirUsage::AllowWrites).IgnoreResult();
    }

    auto& r = ref_msg.GetReader();

    nsStringBuilder tmp;
    nsStringBuilder file, platform;
    nsUInt32 numPermVars;
    nsHybridArray<nsPermutationVar, 16> permVars;

    r >> file;
    r >> platform;
    r >> numPermVars;
    permVars.SetCount(numPermVars);

    tmp.SetFormat("Compiling Shader '{}' - '{}'", file, platform);

    for (auto& pv : permVars)
    {
      r >> pv.m_sName;
      r >> pv.m_sValue;

      tmp.AppendWithSeparator(" | ", pv.m_sName, "=", pv.m_sValue);
    }

    logActivity(tmp);

    // enable runtime shader compilation and set the shader cache directories (this only works, if the user doesn't change the default values)
    // the 'active platform' value should never be used during shader compilation, because there it is passed in
    nsShaderManager::Configure("FILESERVE_UNUSED", true);

    nsLogSystemToBuffer log;
    nsLogSystemScope ls(&log);

    nsShaderCompiler sc;
    nsResult res = sc.CompileShaderPermutationForPlatforms(file, permVars, nsLog::GetThreadLocalLogSystem(), platform);

    nsFileSystem::RemoveDataDirectoryGroup("FileServe");

    if (res.Succeeded())
    {
      // invalidate read cache to not short-circuit the next file read operation
      nsRemoteMessage msg2('FSRV', 'INVC');
      ref_clientChannel.Send(nsRemoteTransmitMode::Reliable, msg2);
    }
    else
    {
      logActivity("[ERROR] Shader Compilation failed:");

      nsHybridArray<nsStringView, 32> lines;
      log.m_sBuffer.Split(false, lines, "\n", "\r");

      for (auto line : lines)
      {
        tmp.Set(">   ", line);
        logActivity(tmp);
      }
    }

    {
      nsRemoteMessage msg2('SHDR', 'CRES');
      msg2.GetWriter() << (res == NS_SUCCESS);
      msg2.GetWriter() << log.m_sBuffer;

      ref_clientChannel.Send(nsRemoteTransmitMode::Reliable, msg2);
    }
  }
}
