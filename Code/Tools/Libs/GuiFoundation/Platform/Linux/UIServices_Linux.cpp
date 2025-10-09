#include <GuiFoundation/GuiFoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

void nsQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;
  nsStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = szPath;
    parentDir = parentDir.GetFileDirectory();
    szPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("xdg-open", args);
}

void nsQtUiServices::OpenWith(const char* szPath)
{
  nsStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.MakePathSeparatorsNative();

  nsLog::Error("nsQtUiServices::OpenWith() not implemented on Linux");
}

nsStatus nsQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  nsLog::Error("nsQtUiServices::OpenInVsCode() not implemented on Linux");
  return nsStatus(NS_FAILURE);
}

#endif
