#pragma once

#include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  struct _EXCEPTION_POINTERS;
}

namespace nsMiniDumpUtils
{
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  NS_FOUNDATION_DLL nsStatus WriteOwnProcessMiniDump(nsStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, nsDumpType dumpTypeOverride = nsDumpType::Auto);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  NS_FOUNDATION_DLL nsMinWindows::HANDLE GetProcessHandleWithNecessaryRights(nsUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  NS_FOUNDATION_DLL nsStatus WriteExternalProcessMiniDump(nsStringView sDumpFile, nsUInt32 uiProcessID, nsMinWindows::HANDLE hProcess, nsDumpType dumpTypeOverride = nsDumpType::Auto);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: A crash-dump with a full memory capture is made if either this application's command line option '-fullcrashdumps' is specified or if that setting is overridden through dumpTypeOverride = nsDumpType::MiniDumpWithFullMemory.
  NS_FOUNDATION_DLL nsStatus WriteProcessMiniDump(nsStringView sDumpFile, nsUInt32 uiProcessID, nsMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, nsDumpType dumpTypeOverrideType = nsDumpType::Auto);

}; // namespace nsMiniDumpUtils
