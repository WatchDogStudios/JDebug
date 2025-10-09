#include <Foundation/FoundationPCH.h>

#if (NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP) && NS_ENABLED(NS_SUPPORTS_DIRECTORY_WATCHER))

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/Containers/DynamicArray.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/Implementation/FileSystemMirror.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

// Comment in to get verbose output on the function of the directory watcher
// #define DEBUG_FILE_WATCHER

#  ifdef DEBUG_FILE_WATCHER
#    define DEBUG_LOG(...) nsLog::Warning(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

namespace
{
  nsCVarBool cvar_ForceNonNTFS("Platform.DirectoryWatcher.ForceNonNTFS", false, nsCVarFlags::Default, "Forces the use of ReadDirectoryChanges instead of ReadDirectoryChangesEx");

  struct MoveEvent
  {
    nsString path;
    bool isDirectory = false;

    void Clear()
    {
      path.Clear();
    }

    bool IsEmpty()
    {
      return path.IsEmpty();
    }
  };

  struct Change
  {
    nsStringBuilder eventFilePath;
    bool isFile;
    DWORD Action;
    LARGE_INTEGER LastModificationTime;
  };

  using nsFileSystemMirrorType = nsFileSystemMirror<bool>;

  void GetChangesNTFS(nsStringView sDirectoryPath, const nsHybridArray<nsUInt8, 4096>& buffer, nsDynamicArray<Change>& ref_changes)
  {
    nsUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((nsUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(uiChanges);
    info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = nsArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        nsHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(nsStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = info->LastModificationTime;
        currentChange.isFile = (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_EXTENDED_INFORMATION*)(((nsUInt8*)info) + info->NextEntryOffset);
    }
  }

  void GetChangesNonNTFS(nsStringView sDirectoryPath, const nsHybridArray<nsUInt8, 4096>& buffer, nsDynamicArray<Change>& ref_changes)
  {
    nsUInt32 uiChanges = 1;
    auto info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();
    while (info->NextEntryOffset != 0)
    {
      uiChanges++;
      info = (const FILE_NOTIFY_INFORMATION*)(((nsUInt8*)info) + info->NextEntryOffset);
    }
    ref_changes.Reserve(ref_changes.GetCount() + uiChanges);
    info = (const FILE_NOTIFY_INFORMATION*)buffer.GetData();

    while (true)
    {
      auto directory = nsArrayPtr<const WCHAR>(info->FileName, info->FileNameLength / sizeof(WCHAR));
      int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), nullptr, 0, nullptr, nullptr);
      if (bytesNeeded > 0)
      {
        nsHybridArray<char, 1024> dir;
        dir.SetCountUninitialized(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, directory.GetPtr(), directory.GetCount(), dir.GetData(), dir.GetCount(), nullptr, nullptr);

        Change& currentChange = ref_changes.ExpandAndGetRef();
        currentChange.eventFilePath = sDirectoryPath;
        currentChange.eventFilePath.AppendPath(nsStringView(dir.GetData(), dir.GetCount()));
        currentChange.eventFilePath.MakeCleanPath();
        currentChange.Action = info->Action;
        currentChange.LastModificationTime = {};
        currentChange.isFile = true; // Pretend it's a file for now.
      }

      if (info->NextEntryOffset == 0)
      {
        break;
      }
      else
        info = (const FILE_NOTIFY_INFORMATION*)(((nsUInt8*)info) + info->NextEntryOffset);
    }
  }

  void PostProcessNonNTFSChanges(nsDynamicArray<Change>& ref_changes, nsFileSystemMirrorType* pMirror)
  {
    nsHybridArray<nsInt32, 4> nextOp;
    // Figure what changes belong to the same object by creating a linked list of changes. This part is tricky as we basically have to handle all the oddities that nsDirectoryWatcher::EnumerateChanges already does again to figure out which operations belong to the same object.
    {
      nsMap<nsStringView, nsUInt32> lastChangeAtPath;
      nextOp.SetCount(ref_changes.GetCount(), -1);

      nsInt32 pendingRemoveOrRename = -1;
      nsInt32 lastMoveFrom = -1;

      for (nsUInt32 i = 0; i < ref_changes.GetCount(); i++)
      {
        const auto& currentChange = ref_changes[i];
        if (pendingRemoveOrRename != -1 && (currentChange.Action == FILE_ACTION_RENAMED_OLD_NAME) && (ref_changes[pendingRemoveOrRename].eventFilePath == currentChange.eventFilePath))
        {
          // This is the bogus removed event because we changed the casing of a file / directory, ignore.
          lastChangeAtPath.Insert(ref_changes[pendingRemoveOrRename].eventFilePath, pendingRemoveOrRename);
          pendingRemoveOrRename = -1;
        }

        if (pendingRemoveOrRename != -1)
        {
          // An actual remove: Stop tracking the change.
          lastChangeAtPath.Remove(ref_changes[pendingRemoveOrRename].eventFilePath);
          pendingRemoveOrRename = -1;
        }

        nsUInt32* uiUniqueItemIndex = nullptr;
        switch (currentChange.Action)
        {
          case FILE_ACTION_ADDED:
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
          case FILE_ACTION_REMOVED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            pendingRemoveOrRename = i;
            break;
          case FILE_ACTION_MODIFIED:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            if (lastChangeAtPath.TryGetValue(currentChange.eventFilePath, uiUniqueItemIndex))
            {
              nextOp[*uiUniqueItemIndex] = i;
              *uiUniqueItemIndex = i;
            }
            else
            {
              lastChangeAtPath[currentChange.eventFilePath] = i;
            }
            lastMoveFrom = i;
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            NS_ASSERT_DEBUG(lastMoveFrom != -1, "last move from should be present when encountering FILE_ACTION_RENAMED_NEW_NAME");
            nextOp[lastMoveFrom] = i;
            lastChangeAtPath.Remove(ref_changes[lastMoveFrom].eventFilePath);
            lastChangeAtPath.Insert(currentChange.eventFilePath, i);
            break;
        }
      }
    }

    // Anything that is chained via the nextOp linked list must be given the same type.
    // Instead of building arrays of arrays, we create a bit field of all changes and then flatten the linked list at the first set bit. While iterating we remove everything we reached via the linked list so on the next call to get the first bit we will find another object that needs processing. As the operations are ordered, the first bit will always point to the very first operation of an object (nextOp can never point to a previous element).
    nsHybridBitfield<128> pendingChanges;
    pendingChanges.SetCount(ref_changes.GetCount(), true);

    // Get start of first object.
    nsHybridArray<Change*, 4> objectChanges;
    auto it = pendingChanges.GetIterator();
    while (it.IsValid())
    {
      // Flatten the changes for one object into a list for easier processing.
      {
        objectChanges.Clear();
        nsUInt32 currentIndex = it.Value();
        objectChanges.PushBack(&ref_changes[currentIndex]);
        pendingChanges.ClearBit(currentIndex);
        while (nextOp[currentIndex] != -1)
        {
          currentIndex = nextOp[currentIndex];
          pendingChanges.ClearBit(currentIndex);
          objectChanges.PushBack(&ref_changes[currentIndex]);
        }
      }

      // Figure out what type the object is. There is no correct way of doing this, which is the reason why ReadDirectoryChangesExW exists. There are however some good heuristics we can use:
      // 1. If the change is on an existing object, we should know it's type from the mirror. This is 100% correct.
      // 2. If object still exists, we can query its stats on disk to determine its type. In rare cases, this can be wrong but you would need to create a case where a file is replaced with a folder of the same name or something.
      // 3. If the object was created and deleted in the same enumeration, we cannot know what type it was, so we take a guess.
      {
        bool isFile = true;
        nsFileSystemMirrorType::Type type;
        if (pMirror->GetType(objectChanges[0]->eventFilePath, type).Succeeded())
        {
          isFile = type == nsFileSystemMirrorType::Type::File;
        }
        else
        {
          bool typeFound = false;
          for (Change* currentChange : objectChanges)
          {
            nsFileStats stats;
            if (nsOSFile::GetFileStats(currentChange->eventFilePath, stats).Succeeded())
            {
              isFile = !stats.m_bIsDirectory;
              typeFound = true;
              break;
            }
          }
          if (!typeFound)
          {
            // No stats and no entry in mirror: It's guessing time!
            isFile = objectChanges[0]->eventFilePath.FindSubString(".") != nullptr;
          }
        }

        // Apply type to all objects in the chain.
        for (Change* currentChange : objectChanges)
        {
          currentChange->isFile = isFile;
        }
      }

      // Find start of next object.
      it = pendingChanges.GetIterator();
    }
  }

} // namespace

struct nsDirectoryWatcherImpl
{
  void DoRead();
  void EnumerateChangesImpl(nsStringView sDirectoryPath, nsTime waitUpTo, const nsDelegate<void(const Change&)>& callback);

  bool m_bNTFS = false;
  HANDLE m_directoryHandle;
  DWORD m_filter;
  OVERLAPPED m_overlapped;
  HANDLE m_overlappedEvent;
  nsDynamicArray<nsUInt8> m_buffer;
  nsBitflags<nsDirectoryWatcher::Watch> m_whatToWatch;
  nsUniquePtr<nsFileSystemMirrorType> m_mirror; // store the last modification timestamp alongside each file
};

nsDirectoryWatcher::nsDirectoryWatcher()
  : m_pImpl(NS_DEFAULT_NEW(nsDirectoryWatcherImpl))
{
  m_pImpl->m_buffer.SetCountUninitialized(1024 * 1024);
}

nsResult nsDirectoryWatcher::OpenDirectory(nsStringView sAbsolutePath, nsBitflags<Watch> whatToWatch)
{
  m_pImpl->m_bNTFS = false;
  {
    // Get drive root:
    nsStringBuilder sTemp = sAbsolutePath;
    sTemp.MakeCleanPath();
    const char* szFirst = sTemp.FindSubString("/");
    NS_ASSERT_DEV(szFirst != nullptr, "The path '{}' is not absolute", sTemp);
    nsStringView sRoot = sAbsolutePath.GetSubString(0, static_cast<nsUInt32>(szFirst - sTemp.GetData()) + 1);

    WCHAR szFileSystemName[8];
    BOOL res = GetVolumeInformationW(nsStringWChar(sRoot),
      nullptr,
      0,
      nullptr,
      nullptr,
      nullptr,
      szFileSystemName,
      NS_ARRAY_SIZE(szFileSystemName));
    m_pImpl->m_bNTFS = res == TRUE && nsStringUtf8(szFileSystemName).GetView() == "NTFS" && !cvar_ForceNonNTFS.GetValue();
  }

  NS_ASSERT_DEV(m_sDirectoryPath.IsEmpty(), "Directory already open, call CloseDirectory first!");
  nsStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim("/");

  m_pImpl->m_whatToWatch = whatToWatch;
  m_pImpl->m_filter = FILE_NOTIFY_CHANGE_FILE_NAME;
  const bool bRequiresMirror = whatToWatch.IsSet(Watch::Writes) || whatToWatch.AreAllSet(Watch::Deletes | Watch::Subdirectories);
  if (bRequiresMirror)
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
  }

  if (!m_pImpl->m_bNTFS || bRequiresMirror)
  {
    m_pImpl->m_mirror = NS_DEFAULT_NEW(nsFileSystemMirrorType);
    m_pImpl->m_mirror->AddDirectory(sPath).AssertSuccess();
  }

  if (whatToWatch.IsAnySet(Watch::Deletes | Watch::Creates | Watch::Renames))
  {
    m_pImpl->m_filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
  }

  m_pImpl->m_directoryHandle = CreateFileW(nsDosDevicePath(sPath), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (m_pImpl->m_directoryHandle == INVALID_HANDLE_VALUE)
  {
    return NS_FAILURE;
  }

  m_pImpl->m_overlappedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (m_pImpl->m_overlappedEvent == INVALID_HANDLE_VALUE)
  {
    return NS_FAILURE;
  }

  m_pImpl->DoRead();
  m_sDirectoryPath = sPath;

  return NS_SUCCESS;
}

void nsDirectoryWatcher::CloseDirectory()
{
  if (!m_sDirectoryPath.IsEmpty())
  {
    CancelIo(m_pImpl->m_directoryHandle);
    CloseHandle(m_pImpl->m_overlappedEvent);
    CloseHandle(m_pImpl->m_directoryHandle);
    m_sDirectoryPath.Clear();
  }
}

nsDirectoryWatcher::~nsDirectoryWatcher()
{
  CloseDirectory();
  NS_DEFAULT_DELETE(m_pImpl);
}

void nsDirectoryWatcherImpl::DoRead()
{
  ResetEvent(m_overlappedEvent);
  memset(&m_overlapped, 0, sizeof(m_overlapped));
  m_overlapped.hEvent = m_overlappedEvent;

  if (m_bNTFS)
  {
    BOOL success =
      ReadDirectoryChangesExW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(nsDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
    NS_ASSERT_DEV(success, "ReadDirectoryChangesExW failed.");
    NS_IGNORE_UNUSED(success);
  }
  else
  {
    BOOL success =
      ReadDirectoryChangesW(m_directoryHandle, m_buffer.GetData(), m_buffer.GetCount(), m_whatToWatch.IsSet(nsDirectoryWatcher::Watch::Subdirectories), m_filter, nullptr, &m_overlapped, nullptr);
    NS_ASSERT_DEV(success, "ReadDirectoryChangesW failed.");
    NS_IGNORE_UNUSED(success);
  }
}

void nsDirectoryWatcherImpl::EnumerateChangesImpl(nsStringView sDirectoryPath, nsTime waitUpTo, const nsDelegate<void(const Change&)>& callback)
{
  nsHybridArray<Change, 6> changes;

  nsHybridArray<nsUInt8, 4096> buffer;
  while (WaitForSingleObject(m_overlappedEvent, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_OBJECT_0)
  {
    waitUpTo = nsTime::MakeZero(); // only wait on the first call to GetQueuedCompletionStatus

    DWORD numberOfBytes = 0;
    GetOverlappedResult(m_directoryHandle, &m_overlapped, &numberOfBytes, FALSE);

    // Copy the buffer
    buffer.SetCountUninitialized(numberOfBytes);
    buffer.GetArrayPtr().CopyFrom(m_buffer.GetArrayPtr().GetSubArray(0, numberOfBytes));

    // Reissue the read request
    DoRead();

    if (numberOfBytes == 0)
    {
      return;
    }

    // We can fire NTFS events right away as they don't need post processing which prevents us from resizing the changes array unnecessarily.
    if (m_bNTFS)
    {
      GetChangesNTFS(sDirectoryPath, buffer, changes);
      for (const Change& change : changes)
      {
        callback(change);
      }
      changes.Clear();
    }
    else
    {
      GetChangesNonNTFS(sDirectoryPath, buffer, changes);
    }
  }

  // Non-NTFS changes need to be collected and processed in one go to be able to reconstruct the type of the change.
  if (!m_bNTFS)
  {
    PostProcessNonNTFSChanges(changes, m_mirror.Borrow());
    for (const Change& change : changes)
    {
      callback(change);
    }
  }
}

void nsDirectoryWatcher::EnumerateChanges(EnumerateChangesFunction func, nsTime waitUpTo)
{
  nsFileSystemMirrorType* mirror = m_pImpl->m_mirror.Borrow();
  NS_ASSERT_DEV(!m_sDirectoryPath.IsEmpty(), "No directory opened!");

  MoveEvent pendingRemoveOrRename;
  const nsBitflags<nsDirectoryWatcher::Watch> whatToWatch = m_pImpl->m_whatToWatch;
  // Renaming a file to the same filename with different casing triggers the events REMOVED (old casing) -> RENAMED_OLD_NAME -> _RENAMED_NEW_NAME.
  // Thus, we need to cache every remove event to make sure the very next event is not a rename of the exact same file.
  auto FirePendingRemove = [&]()
  {
    if (!pendingRemoveOrRename.IsEmpty())
    {
      if (pendingRemoveOrRename.isDirectory)
      {
        if (whatToWatch.IsSet(Watch::Deletes))
        {
          if (mirror && whatToWatch.IsSet(Watch::Subdirectories))
          {
            mirror->Enumerate(pendingRemoveOrRename.path, [&](const nsStringBuilder& sPath, nsFileSystemMirrorType::Type type)
                    { func(sPath, nsDirectoryWatcherAction::Removed, (type == nsFileSystemMirrorType::Type::File) ? nsDirectoryWatcherType::File : nsDirectoryWatcherType::Directory); })
              .AssertSuccess();
          }
          func(pendingRemoveOrRename.path, nsDirectoryWatcherAction::Removed, nsDirectoryWatcherType::Directory);
        }
        if (mirror)
        {
          mirror->RemoveDirectory(pendingRemoveOrRename.path).AssertSuccess();
        }
      }
      else
      {
        if (mirror)
        {
          mirror->RemoveFile(pendingRemoveOrRename.path).AssertSuccess();
        }
        if (whatToWatch.IsSet(nsDirectoryWatcher::Watch::Deletes))
        {
          func(pendingRemoveOrRename.path, nsDirectoryWatcherAction::Removed, nsDirectoryWatcherType::File);
        }
      }
      pendingRemoveOrRename.Clear();
    }
  };

  NS_SCOPE_EXIT(FirePendingRemove());

  MoveEvent lastMoveFrom;

  // Process the messages
  m_pImpl->EnumerateChangesImpl(m_sDirectoryPath, waitUpTo, [&](const Change& info)
    {
      nsDirectoryWatcherAction action = nsDirectoryWatcherAction::None;
      bool fireEvent = false;

      if (!pendingRemoveOrRename.IsEmpty() && info.isFile == !pendingRemoveOrRename.isDirectory && info.Action == FILE_ACTION_RENAMED_OLD_NAME && pendingRemoveOrRename.path == info.eventFilePath)
      {
        // This is the bogus removed event because we changed the casing of a file / directory, ignore.
        pendingRemoveOrRename.Clear();
      }
      FirePendingRemove();

      if (info.isFile)
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
            DEBUG_LOG("FILE_ACTION_ADDED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = nsDirectoryWatcherAction::Added;
            fireEvent = whatToWatch.IsSet(nsDirectoryWatcher::Watch::Creates);
            if (mirror)
            {
              bool fileAlreadyExists = false;
              mirror->AddFile(info.eventFilePath, true, &fileAlreadyExists, nullptr).AssertSuccess();
              if (fileAlreadyExists)
              {
                fireEvent = false;
              }
            }
            break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("FILE_ACTION_REMOVED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = nsDirectoryWatcherAction::Removed;
            fireEvent = false;
            pendingRemoveOrRename = {info.eventFilePath, false};
            break;
          case FILE_ACTION_MODIFIED:
          {
            DEBUG_LOG("FILE_ACTION_MODIFIED {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = nsDirectoryWatcherAction::Modified;
            fireEvent = whatToWatch.IsAnySet(nsDirectoryWatcher::Watch::Writes);
            bool fileAreadyKnown = false;
            bool addPending = false;
            if (mirror)
            {
              mirror->AddFile(info.eventFilePath, false, &fileAreadyKnown, &addPending).AssertSuccess();
            }
            if (fileAreadyKnown && addPending)
            {
              fireEvent = false;
            }
          }
          break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_OLD_NAME {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            NS_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            action = nsDirectoryWatcherAction::RenamedOldName;
            fireEvent = whatToWatch.IsAnySet(nsDirectoryWatcher::Watch::Renames);
            NS_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending last move from");
            lastMoveFrom = {info.eventFilePath, false};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("FILE_ACTION_RENAMED_NEW_NAME {} ({})", info.eventFilePath, info.LastModificationTime.QuadPart);
            action = nsDirectoryWatcherAction::RenamedNewName;
            fireEvent = whatToWatch.IsAnySet(nsDirectoryWatcher::Watch::Renames);
            NS_ASSERT_DEV(!lastMoveFrom.IsEmpty() && !lastMoveFrom.isDirectory, "last move from doesn't match");
            if (mirror)
            {
              mirror->RemoveFile(lastMoveFrom.path).AssertSuccess();
              mirror->AddFile(info.eventFilePath, false, nullptr, nullptr).AssertSuccess();
            }
            lastMoveFrom.Clear();
            break;
        }

        if (fireEvent)
        {
          func(info.eventFilePath, action, nsDirectoryWatcherType::File);
        }
      }
      else
      {
        switch (info.Action)
        {
          case FILE_ACTION_ADDED:
          {
            DEBUG_LOG("DIR_ACTION_ADDED {}", info.eventFilePath);
            bool directoryAlreadyKnown = false;
            if (mirror)
            {
              mirror->AddDirectory(info.eventFilePath, &directoryAlreadyKnown).AssertSuccess();
            }

            if (whatToWatch.IsSet(Watch::Creates) && !directoryAlreadyKnown)
            {
              func(info.eventFilePath, nsDirectoryWatcherAction::Added, nsDirectoryWatcherType::Directory);
            }

            // Whenever we add a directory we might be "to late" to see changes inside it.
            // So iterate the file system and make sure we track all files / subdirectories
            nsFileSystemIterator subdirIt;

            subdirIt.StartSearch(info.eventFilePath.GetData(),
              whatToWatch.IsSet(nsDirectoryWatcher::Watch::Subdirectories)
                ? nsFileSystemIteratorFlags::ReportFilesAndFoldersRecursive
                : nsFileSystemIteratorFlags::ReportFiles);

            nsStringBuilder tmpPath2;
            for (; subdirIt.IsValid(); subdirIt.Next())
            {
              const nsFileStats& stats = subdirIt.GetStats();
              stats.GetFullPath(tmpPath2);
              if (stats.m_bIsDirectory)
              {
                directoryAlreadyKnown = false;
                if (mirror)
                {
                  mirror->AddDirectory(tmpPath2, &directoryAlreadyKnown).AssertSuccess();
                }
                if (whatToWatch.IsSet(nsDirectoryWatcher::Watch::Creates) && !directoryAlreadyKnown)
                {
                  func(tmpPath2, nsDirectoryWatcherAction::Added, nsDirectoryWatcherType::Directory);
                }
              }
              else
              {
                bool fileExistsAlready = false;
                if (mirror)
                {
                  mirror->AddFile(tmpPath2, false, &fileExistsAlready, nullptr).AssertSuccess();
                }
                if (whatToWatch.IsSet(nsDirectoryWatcher::Watch::Creates) && !fileExistsAlready)
                {
                  func(tmpPath2, nsDirectoryWatcherAction::Added, nsDirectoryWatcherType::File);
                }
              }
            }
          }
          break;
          case FILE_ACTION_REMOVED:
            DEBUG_LOG("DIR_ACTION_REMOVED {}", info.eventFilePath);
            pendingRemoveOrRename = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
            DEBUG_LOG("DIR_ACTION_OLD_NAME {}", info.eventFilePath);
            NS_ASSERT_DEV(lastMoveFrom.IsEmpty(), "there should be no pending move from");
            lastMoveFrom = {info.eventFilePath, true};
            break;
          case FILE_ACTION_RENAMED_NEW_NAME:
            DEBUG_LOG("DIR_ACTION_NEW_NAME {}", info.eventFilePath);
            NS_ASSERT_DEV(!lastMoveFrom.IsEmpty(), "rename old name and rename new name should always appear in pairs");
            if (mirror)
            {
              mirror->MoveDirectory(lastMoveFrom.path, info.eventFilePath).AssertSuccess();
            }
            if (whatToWatch.IsSet(Watch::Renames))
            {
              func(lastMoveFrom.path, nsDirectoryWatcherAction::RenamedOldName, nsDirectoryWatcherType::Directory);
              func(info.eventFilePath, nsDirectoryWatcherAction::RenamedNewName, nsDirectoryWatcherType::Directory);
            }
            lastMoveFrom.Clear();
            break;
          default:
            break;
        }
      } //
    });
}


void nsDirectoryWatcher::EnumerateChanges(nsArrayPtr<nsDirectoryWatcher*> watchers, EnumerateChangesFunction func, nsTime waitUpTo)
{
  nsHybridArray<HANDLE, 16> events;
  events.SetCount(watchers.GetCount());

  for (nsUInt32 i = 0; i < watchers.GetCount(); ++i)
  {
    events[i] = watchers[i]->m_pImpl->m_overlappedEvent;
  }

  // Wait for any of the watchers to have some data ready
  if (WaitForMultipleObjects(events.GetCount(), events.GetData(), FALSE, static_cast<DWORD>(waitUpTo.GetMilliseconds())) == WAIT_TIMEOUT)
  {
    return;
  }

  // Iterate all of them to make sure we report all changes up to this point.
  for (nsDirectoryWatcher* watcher : watchers)
  {
    watcher->EnumerateChanges(func);
  }
}

#endif


NS_STATICLINK_FILE(Foundation, Foundation_Platform_Win_DirectoryWatcher_Win);
