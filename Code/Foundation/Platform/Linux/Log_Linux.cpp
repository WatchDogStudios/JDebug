#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_LINUX)

#  include <Foundation/Logging/ETW.h>
#  include <Foundation/Logging/Log.h>

void nsLog::Print(const char* szText)
{
  printf("%s", szText);

  nsETW::LogMessage(nsLogMsgType::ErrorMsg, 0, szText);

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

  nsLog::Print(display);
  NS_ASSERT_NOT_IMPLEMENTED;
}

#endif
