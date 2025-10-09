#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)

#  include <Foundation/Strings/FormatString.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Strings/StringBuilder.h>

#  include <string.h>

nsStringView BuildString(char* szTmp, nsUInt32 uiLength, const nsArgErrno& arg)
{
  const char* szErrorMsg = std::strerror(arg.m_iErrno);
  nsStringUtils::snprintf(szTmp, uiLength, "%i (\"%s\")", arg.m_iErrno, szErrorMsg);
  return nsStringView(szTmp);
}
#endif
