#include <GuiFoundation/GuiFoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)

#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <ToolsFoundation/Application/ApplicationServices.h>
#  include <ToolsFoundation/Project/ToolsProject.h>

#  include <ShlObj_core.h>
#include <Foundation/IO/OSFile.h>

void nsQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;

  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
}

void nsQtUiServices::OpenWith(const char* szPath)
{
  nsStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.MakePathSeparatorsNative();

  nsStringWChar wpath(sPath);
  OPENASINFO oi;
  oi.pcszFile = wpath.GetData();
  oi.pcszClass = NULL;
  oi.oaifInFlags = OAIF_EXEC;
  SHOpenWithDialog(NULL, &oi);
}

nsStatus nsQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe;

  {
    nsStringBuilder sDstDir = nsToolsProject::GetSingleton()->GetProjectDirectory();
    sDstDir.AppendPath(".vscode");

    nsStringBuilder sSrcDir = nsApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sSrcDir.AppendPath("VSC");
    nsOSFile::CopyFolder(sSrcDir, sDstDir).IgnoreResult();
  }

  sVsCodeExe = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

  if (!QFile().exists(sVsCodeExe))
  {
    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", QSettings::NativeFormat);
    QString sVsCodeExeKey = settings.value(".", "").value<QString>();

    if (sVsCodeExeKey.length() > 5)
    {
      // Remove shell parameter and normalize QT Compatible path, QFile expects the file separator to be '/' regardless of operating system
      sVsCodeExe = sVsCodeExeKey.left(sVsCodeExeKey.length() - 5).replace("\\", "/").replace("\"", "");
    }
  }

  if (sVsCodeExe.isEmpty() || !QFile().exists(sVsCodeExe))
  {
    // Try code executable in PATH
    if (QProcess::execute("code", {"--version"}) == 0)
    {
      sVsCodeExe = "code";
    }
    else
    {
      return nsStatus("Installation of Visual Studio Code could not be located.\n"
                      "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
    }
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return nsStatus("Failed to launch Visual Studio Code.");
  }

  return nsStatus(NS_SUCCESS);
}


#endif
