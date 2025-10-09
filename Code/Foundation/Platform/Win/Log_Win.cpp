#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Logging/ETW.h>
#  include <Foundation/Logging/Log.h>

void nsLog::Print(const char* szText)
{
  printf("%s", szText);

  nsETW::LogMessage(nsLogMsgType::ErrorMsg, 0, szText);
  OutputDebugStringW(nsStringWChar(szText).GetData());

  if (s_CustomPrintFunction)
  {
    s_CustomPrintFunction(szText);
  }

  fflush(stdout);
  fflush(stderr);
}

void nsLog::OsMessageBox(const nsFormatString& text)
{
  nsStringBuilder tmp;
  nsStringBuilder display = text.GetText(tmp);
  display.Trim(" \n\r\t");

#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
  const char* title = "";
  if (nsApplication::GetApplicationInstance())
  {
    title = nsApplication::GetApplicationInstance()->GetApplicationName();
  }

  MessageBoxW(nullptr, nsStringWChar(display).GetData(), nsStringWChar(title), MB_OK);
#  else
  nsLog::Print(display);
  NS_ASSERT_NOT_IMPLEMENTED;
#  endif
}

#endif
