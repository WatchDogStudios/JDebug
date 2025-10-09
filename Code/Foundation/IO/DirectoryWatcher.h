#pragma once

#include <Foundation/Basics.h>

#if NS_ENABLED(NS_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct nsDirectoryWatcherImpl;

/// \brief Which action has been performed on a file.
enum class nsDirectoryWatcherAction
{
  None,           ///< Nothing happened
  Added,          ///< A file or directory was added, requires nsDirectoryWatcher::Watch::Creates flag when creating nsDirectoryWatcher.
  Removed,        ///< A file or directory was removed, requires nsDirectoryWatcher::Watch::Deletes flag when creating nsDirectoryWatcher.
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file, requires nsDirectoryWatcher::Watch::Writes flag when creating nsDirectoryWatcher.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided, requires nsDirectoryWatcher::Watch::Renames flag when creating nsDirectoryWatcher.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second, requires nsDirectoryWatcher::Watch::Renames flag when creating nsDirectoryWatcher.
};

enum class nsDirectoryWatcherType
{
  File,
  Directory
};

/// \brief Platform-abstracted file system monitoring for detecting directory changes
///
/// Provides efficient monitoring of file system events (create, modify, delete, rename) within
/// a specified directory. Uses native OS facilities (inotify on Linux, ReadDirectoryChangesW on Windows)
/// for minimal overhead polling. Essential for tools like asset hot-reloading, live file editing,
/// and build system dependency tracking.
///
/// Changes are queued internally and retrieved through polling via EnumerateChanges(). Supports
/// configurable event filtering and optional recursive subdirectory monitoring.
class NS_FOUNDATION_DLL nsDirectoryWatcher
{
public:
  /// \brief What to watch out for.
  struct Watch
  {
    using StorageType = nsUInt8;
    constexpr static nsUInt8 Default = 0;

    /// \brief Enum values
    enum Enum
    {
      Writes = NS_BIT(0),         ///< Watch for writes. Will trigger nsDirectoryWatcherAction::Modified events.
      Creates = NS_BIT(1),        ///< Watch for newly created files. Will trigger nsDirectoryWatcherAction::Added events.
      Deletes = NS_BIT(2),        ///< Watch for deleted files. Will trigger nsDirectoryWatcherAction::Removed events.
      Renames = NS_BIT(3),        ///< Watch for renames. Will trigger nsDirectoryWatcherAction::RenamedOldName and nsDirectoryWatcherAction::RenamedNewName events.
      Subdirectories = NS_BIT(4), ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Writes : 1;
      StorageType Creates : 1;
      StorageType Deletes : 1;
      StorageType Renames : 1;
      StorageType Subdirectories : 1;
    };
  };

  nsDirectoryWatcher();
  nsDirectoryWatcher(const nsDirectoryWatcher&) = delete;
  nsDirectoryWatcher(nsDirectoryWatcher&&) noexcept = delete;
  ~nsDirectoryWatcher();

  nsDirectoryWatcher& operator=(const nsDirectoryWatcher&) = delete;
  nsDirectoryWatcher& operator=(nsDirectoryWatcher&&) noexcept = delete;

  /// \brief
  ///   Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of nsDirectoryWatcher can only watch one directory at a time.
  nsResult OpenDirectory(nsStringView sAbsolutePath, nsBitflags<Watch> whatToWatch);

  /// \brief
  ///   Closes the currently watched directory if any.
  void CloseDirectory();

  /// \brief
  ///   Returns the opened directory, will be empty if no directory was opened.
  nsStringView GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = nsDelegate<void(nsStringView sFilename, nsDirectoryWatcherAction action, nsDirectoryWatcherType type), 48>;

  /// \brief
  ///   Calls the callback \p func for each change since the last call. For each change the filename
  ///   and the action, which was performed on the file, is passed to \p func.
  ///   If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, nsTime waitUpTo = nsTime::MakeZero());

  /// \brief
  ///   Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(nsArrayPtr<nsDirectoryWatcher*> watchers, EnumerateChangesFunction func, nsTime waitUpTo = nsTime::MakeZero());

private:
  nsString m_sDirectoryPath;
  nsDirectoryWatcherImpl* m_pImpl = nullptr;
};

NS_DECLARE_FLAGS_OPERATORS(nsDirectoryWatcher::Watch);

#endif
