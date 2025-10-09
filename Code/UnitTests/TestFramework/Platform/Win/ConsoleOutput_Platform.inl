#include <Foundation/Logging/ETW.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

void OutputToConsole_Platform(nsUInt8 uiColor, nsTestOutput::Enum type, nsInt32 iIndentation, const char* szMsg)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)uiColor);
  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);

  char sz[4096];
  nsStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(nsStringWChar(sz).GetData());
}
