#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Types/Status.h>

/// \brief Functionality for writing process mini-dumps (callstacks, memory snapshots, etc)
namespace nsMiniDumpUtils
{
  /// \brief Specifies the dump mode that is written.
  enum class nsDumpType
  {
    Auto,                  ///< Uses the setting specified globally through the command line.
    MiniDump,              ///< Saves a mini-dump without full memory, regardless of this application's command line flag '-fullcrashdumps'.
    MiniDumpWithFullMemory ///< Saves a mini-dump with full memory, regardless of this application's command line flag '-fullcrashdumps'.
  };

  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  NS_FOUNDATION_DLL nsStatus WriteExternalProcessMiniDump(nsStringView sDumpFile, nsUInt32 uiProcessID, nsDumpType dumpTypeOverride = nsDumpType::Auto);

  /// \brief Tries to launch ns's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: The command line option '-fullcrashdumps' is passed if either set in this application's command line or if overridden through dumpTypeOverride = nsDumpType::MiniDumpWithFullMemory.
  NS_FOUNDATION_DLL nsStatus LaunchMiniDumpTool(nsStringView sDumpFile, nsDumpType dumpTypeOverride = nsDumpType::Auto);

}; // namespace nsMiniDumpUtils


#if NS_ENABLED(NS_SUPPORTS_CRASH_DUMPS)

#  include <MiniDumpUtils_Platform.h>

#endif
