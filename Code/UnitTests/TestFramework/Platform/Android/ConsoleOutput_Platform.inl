#include <android/log.h>

void OutputToConsole_Platform(nsUInt8 uiColor, nsTestOutput::Enum type, nsInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
  __android_log_print(ANDROID_LOG_DEBUG, "WDFramework", "%*s%s\n", iIndentation, "", szMsg);
}
