#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/ApplicationEntryPoint_Platform.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

namespace nsApplicationDetails
{
  void SetConsoleCtrlHandler(nsMinWindows::BOOL(NS_WINDOWS_WINAPI* consoleHandler)(nsMinWindows::DWORD dwCtrlType))
  {
    ::SetConsoleCtrlHandler(consoleHandler, TRUE);
  }

  static nsMutex s_shutdownMutex;

  nsMutex& GetShutdownMutex()
  {
    return s_shutdownMutex;
  }

} // namespace nsApplicationDetails
#endif
