#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/ConversionUtils.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

bool nsDefaultAssertHandler_Platform(const char* szSourceFile, nsUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp);

#include <Assert_Platform.inl>

bool nsDefaultAssertHandler(const char* szSourceFile, nsUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  char szTemp[1024 * 4] = "";
  nsStringUtils::snprintf(szTemp, NS_ARRAY_SIZE(szTemp),
    "\n\n *** Assertion ***\n\n    Expression: \"%s\"\n    Function: \"%s\"\n    File: \"%s\"\n    Line: %u\n    Message: \"%s\"\n\n", szExpression,
    szFunction, szSourceFile, uiLine, szAssertMsg);
  szTemp[1024 * 4 - 1] = '\0';

  nsLog::Print(szTemp);

  if (nsSystemInformation::IsDebuggerAttached())
    return true;

  // If no debugger is attached we append the assert to a common file so that postmortem debugging is easier
  if (FILE* assertLogFP = fopen("nsDefaultAssertHandlerOutput.txt", "a"))
  {
    time_t timeUTC = time(&timeUTC);
    tm* ptm = gmtime(&timeUTC);

    char szTimeStr[256] = {0};
    nsStringUtils::snprintf(szTimeStr, 256, "UTC: %s", asctime(ptm));
    fputs(szTimeStr, assertLogFP);

    fputs(szTemp, assertLogFP);

    fclose(assertLogFP);
  }

  // if the environment variable "NS_SILENT_ASSERTS" is set to a value like "1", "on", "true", "enable" or "yes"
  // the assert handler will never show a GUI that may block the application from continuing to run
  // this should be set on machines that run tests which should never get stuck but rather crash asap
  bool bSilentAsserts = false;

  if (nsEnvironmentVariableUtils::IsVariableSet("NS_SILENT_ASSERTS"))
  {
    bSilentAsserts = nsEnvironmentVariableUtils::GetValueInt("NS_SILENT_ASSERTS", bSilentAsserts ? 1 : 0) != 0;
  }

  if (bSilentAsserts)
    return true;

  return nsDefaultAssertHandler_Platform(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg, szTemp);
}

static nsAssertHandler g_AssertHandler = &nsDefaultAssertHandler;

nsAssertHandler nsGetAssertHandler()
{
  return g_AssertHandler;
}

void nsSetAssertHandler(nsAssertHandler handler)
{
  g_AssertHandler = handler;
}

bool nsFailedCheck(const char* szSourceFile, nsUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}

bool nsFailedCheck(const char* szSourceFile, nsUInt32 uiLine, const char* szFunction, const char* szExpression, const class nsFormatString& msg)
{
  nsStringBuilder tmp;
  return nsFailedCheck(szSourceFile, uiLine, szFunction, szExpression, msg.GetTextCStr(tmp));
}
