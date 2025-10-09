#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)

#  include <Foundation/Logging/Log.h>

#  include <android/log.h>

void nsLog::Print(const char* szText)
{
  printf("%s", szText);

  __android_log_print(ANDROID_LOG_ERROR, "WDFramework", "%s", szText);

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
