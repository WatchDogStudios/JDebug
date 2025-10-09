#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef NS_POSIX_FILE_USEOLDAPI
#  include <direct.h>
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

nsResult nsOSFile::InternalOpen(nsStringView sFile, nsFileOpenMode::Enum OpenMode, nsFileShareMode::Enum FileShareMode)
{
  nsStringBuilder sFileCopy = sFile;
  const char* szFile = sFileCopy;

#ifndef NS_POSIX_FILE_USEOLDAPI // UWP does not support these functions
  int fd = -1;
  switch (OpenMode)
  {
    // O_CLOEXEC = don't forward to child processes
    case nsFileOpenMode::Read:
      fd = open(szFile, O_RDONLY | O_CLOEXEC);
      break;
    case nsFileOpenMode::Write:
    case nsFileOpenMode::Append:
      fd = open(szFile, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
      break;
    default:
      break;
  }

  if (FileShareMode == nsFileShareMode::Default)
  {
    if (OpenMode == nsFileOpenMode::Read)
    {
      FileShareMode = nsFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = nsFileShareMode::Exclusive;
    }
  }

  if (fd == -1)
  {
    return NS_FAILURE;
  }

  struct stat stats = {};
  if (fstat(fd, &stats) != 0)
  {
    close(fd);
    return NS_FAILURE;
  }

  // Prevent opening of directories
  if ((stats.st_mode & S_IFMT) == S_IFDIR)
  {
    close(fd);
    return NS_FAILURE;
  }

  const int iSharedMode = (FileShareMode == nsFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;
  const nsTime sleepTime = nsTime::MakeFromMilliseconds(20);
  nsInt32 iRetries = m_bRetryOnSharingViolation ? 20 : 1;

  while (flock(fd, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    int errorCode = errno;
    iRetries--;
    if (iRetries == 0 || errorCode != EWOULDBLOCK)
    {
      // error, could not get a lock
      close(fd);
      return NS_FAILURE;
    }
    nsThreadUtils::Sleep(sleepTime);
  }

  switch (OpenMode)
  {
    case nsFileOpenMode::Read:
      m_FileData.m_pFileHandle = fdopen(fd, "rb");
      break;
    case nsFileOpenMode::Write:
      if (ftruncate(fd, 0) < 0)
      {
        close(fd);
        return NS_FAILURE;
      }
      m_FileData.m_pFileHandle = fdopen(fd, "wb");
      break;
    case nsFileOpenMode::Append:
      m_FileData.m_pFileHandle = fdopen(fd, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, nsFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
  {
    close(fd);
  }

#else
  NS_IGNORE_UNUSED(FileShareMode);

  switch (OpenMode)
  {
    case nsFileOpenMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case nsFileOpenMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case nsFileOpenMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, nsFileSeekMode::FromEnd);

      break;
    default:
      break;
  }
#endif

  if (m_FileData.m_pFileHandle == nullptr)
  {
    return NS_FAILURE;
  }

  // lock will be released automatically when the file is closed
  return NS_SUCCESS;
}

void nsOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

nsResult nsOSFile::InternalWrite(const void* pBuffer, nsUInt64 uiBytes)
{
  const nsUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
    {
      return NS_FAILURE;
    }

    uiBytes -= uiBatchBytes;
    pBuffer = nsMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const nsUInt32 uiBytes32 = static_cast<nsUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
    {
      return NS_FAILURE;
    }
  }

  return NS_SUCCESS;
}

nsUInt64 nsOSFile::InternalRead(void* pBuffer, nsUInt64 uiBytes)
{
  nsUInt64 uiBytesRead = 0;

  const nsUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    const nsUInt64 uiReadThisTime = fread(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle);
    uiBytesRead += uiReadThisTime;

    if (uiReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = nsMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const nsUInt32 uiBytes32 = static_cast<nsUInt32>(uiBytes);

    uiBytesRead += fread(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle);
  }

  return uiBytesRead;
}

nsUInt64 nsOSFile::InternalGetFilePosition() const
{
#ifdef NS_POSIX_FILE_USEOLDAPI
  return static_cast<nsUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<nsUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void nsOSFile::InternalSetFilePosition(nsInt64 iDistance, nsFileSeekMode::Enum Pos) const
{
#ifdef NS_POSIX_FILE_USEOLDAPI
  switch (Pos)
  {
    case nsFileSeekMode::FromStart:
      NS_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case nsFileSeekMode::FromEnd:
      NS_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case nsFileSeekMode::FromCurrent:
      NS_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case nsFileSeekMode::FromStart:
      NS_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case nsFileSeekMode::FromEnd:
      NS_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case nsFileSeekMode::FromCurrent:
      NS_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#endif
}

// this might not be defined on Windows
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

bool nsOSFile::InternalExistsFile(nsStringView sFile)
{
  struct stat sb;
  return (stat(nsString(sFile), &sb) == 0 && !S_ISDIR(sb.st_mode));
}

bool nsOSFile::InternalExistsDirectory(nsStringView sDirectory)
{
  struct stat sb;
  return (stat(nsString(sDirectory), &sb) == 0 && S_ISDIR(sb.st_mode));
}

nsResult nsOSFile::InternalDeleteFile(nsStringView sFile)
{
#ifdef NS_POSIX_FILE_USEWINDOWSAPI
  int iRes = _unlink(nsString(sFile));
#else
  int iRes = unlink(nsString(sFile));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return NS_SUCCESS;

  return NS_FAILURE;
}

nsResult nsOSFile::InternalDeleteDirectory(nsStringView sDirectory)
{
#ifdef NS_POSIX_FILE_USEWINDOWSAPI
  int iRes = _rmdir(nsString(sDirectory));
#else
  int iRes = rmdir(nsString(sDirectory));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return NS_SUCCESS;

  return NS_FAILURE;
}

nsResult nsOSFile::InternalCreateDirectory(nsStringView sDirectory)
{
  // handle drive letters as always successful
  if (nsStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 1) // '/'
    return NS_SUCCESS;

#ifdef NS_POSIX_FILE_USEWINDOWSAPI
  int iRes = _mkdir(nsString(sDirectory));
#else
  int iRes = mkdir(nsString(sDirectory), 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return NS_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to nsOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
  if (errno == EACCES && InternalExistsDirectory(sDirectory))
    return NS_SUCCESS;

  return NS_FAILURE;
}

nsResult nsOSFile::InternalMoveFileOrDirectory(nsStringView sDirectoryFrom, nsStringView sDirectoryTo)
{
  if (rename(nsString(sDirectoryFrom), nsString(sDirectoryTo)) != 0)
  {
    return NS_FAILURE;
  }
  return NS_SUCCESS;
}

#if NS_ENABLED(NS_SUPPORTS_FILE_STATS)

#  ifndef NS_POSIX_FILE_NOINTERNALGETFILESTATS

nsResult nsOSFile::InternalGetFileStats(nsStringView sFileOrFolder, nsFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(nsString(sFileOrFolder), &tempStat);

  if (iRes != 0)
    return NS_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = nsPathUtils::GetFileNameAndExtension(sFileOrFolder); // no OS support, so just pass it through
#    ifdef __USE_XOPEN2K8
  out_Stats.m_LastModificationTime = nsTimestamp::MakeFromInt(tempStat.st_mtim.tv_sec * 1000000000ull + tempStat.st_mtim.tv_nsec, nsSIUnitOfTime::Nanosecond);
#    else
  out_Stats.m_LastModificationTime = nsTimestamp::MakeFromInt(tempStat.st_mtime, nsSIUnitOfTime::Second);
#    endif
  return NS_SUCCESS;
}

#  endif

#endif

#ifndef NS_POSIX_FILE_NOGETAPPLICATIONPATH

nsStringView nsOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    char result[PATH_MAX];
    size_t length = readlink("/proc/self/exe", result, PATH_MAX);
    s_sApplicationPath = nsStringView(result, result + length);
  }

  return s_sApplicationPath;
}

#endif

#ifndef NS_POSIX_FILE_NOGETUSERDATAFOLDER

nsString nsOSFile::GetUserDataFolder(nsStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    nsStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath(".local", "share");
    s_sUserDataPath = sTemp;
  }

  nsStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef NS_POSIX_FILE_NOGETTEMPDATAFOLDER

nsString nsOSFile::GetTempDataFolder(nsStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    nsStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath(".cache");
    s_sTempDataPath = sTemp;
  }

  nsStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef NS_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

nsString nsOSFile::GetUserDocumentsFolder(nsStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    nsStringBuilder sTemp = getenv("HOME");

    if (sTemp.IsEmpty())
      sTemp = getpwuid(getuid())->pw_dir;

    sTemp.AppendPath("Documents");
    s_sUserDocumentsPath = sTemp;
  }

  nsStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif

#ifndef NS_POSIX_FILE_NOGETCURRENTWORKINGDIRECTORY

const nsString nsOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  nsStringBuilder clean = getcwd(tmp, NS_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#endif
