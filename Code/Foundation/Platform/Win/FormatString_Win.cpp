#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>

nsStringView BuildString(char* szTmp, nsUInt32 uiLength, const nsArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr) == 0)
  {
    DWORD err = GetLastError();
    nsStringUtils::snprintf(szTmp, uiLength, "%u (FormatMessageW failed with error code %u)", arg.m_ErrorCode, err);
    return nsStringView(szTmp);
  }

  LPWSTR pCRLF = wcschr((LPWSTR)lpMsgBuf, L'\r');
  if (pCRLF != nullptr)
  {
    // remove the \r\n that FormatMessageW always appends
    *pCRLF = L'\0';
  }

  // we need a bigger boat
  static thread_local char FullMessage[256];

  nsStringUtils::snprintf(FullMessage, NS_ARRAY_SIZE(FullMessage), "%u (\"%s\")", arg.m_ErrorCode, nsStringUtf8((LPWSTR)lpMsgBuf).GetData());
  LocalFree(lpMsgBuf);
  return nsStringView(FullMessage);
}

#endif
