#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

#  include <Shlobj.h>

nsResult nsOSFile::InternalOpen(nsStringView sFile, nsFileOpenMode::Enum OpenMode, nsFileShareMode::Enum FileShareMode)
{
  const nsTime sleepTime = nsTime::MakeFromMilliseconds(20);
  nsInt32 iRetries = 20;

  if (FileShareMode == nsFileShareMode::Default)
  {
    // when 'default' share mode is requested, use 'share reads' when opening a file for reading
    // and use 'exclusive' when opening a file for writing

    if (OpenMode == nsFileOpenMode::Read)
    {
      FileShareMode = nsFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = nsFileShareMode::Exclusive;
    }
  }

  DWORD dwSharedMode = 0; // exclusive access
  if (FileShareMode == nsFileShareMode::SharedReads)
  {
    dwSharedMode = FILE_SHARE_READ;
  }

  while (iRetries > 0)
  {
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;

    switch (OpenMode)
    {
      case nsFileOpenMode::Read:
        m_FileData.m_pFileHandle = CreateFileW(nsDosDevicePath(sFile), GENERIC_READ, dwSharedMode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case nsFileOpenMode::Write:
        m_FileData.m_pFileHandle = CreateFileW(nsDosDevicePath(sFile), GENERIC_WRITE, dwSharedMode, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case nsFileOpenMode::Append:
        m_FileData.m_pFileHandle = CreateFileW(nsDosDevicePath(sFile), FILE_APPEND_DATA, dwSharedMode, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
        if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
          InternalSetFilePosition(0, nsFileSeekMode::FromEnd);

        break;

        NS_DEFAULT_CASE_NOT_IMPLEMENTED
    }

    const nsResult res = ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? NS_SUCCESS : NS_FAILURE;

    if (res.Failed())
    {
      if (nsOSFile::ExistsDirectory(sFile))
      {
        // trying to 'open' a directory fails with little useful error codes such as 'access denied'
        return NS_FAILURE;
      }

      error = GetLastError();

      // file does not exist
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return res;
      // badly formed path, happens when two absolute paths are concatenated
      if (error == ERROR_INVALID_NAME)
        return res;

      if (error == ERROR_SHARING_VIOLATION
          // these two situations happen when the nsInspector is connected
          // for some reason, the networking blocks file reading (when run on the same machine)
          // retrying fixes the problem, but can introduce very long stalls
          || error == WSAEWOULDBLOCK || error == ERROR_SUCCESS)
      {
        if (m_bRetryOnSharingViolation)
        {
          --iRetries;
          nsThreadUtils::Sleep(sleepTime);
          continue; // try again
        }
        else
        {
          return res;
        }
      }

      // anything else, print an error (for now)
      nsLog::Error("CreateFile failed with error {0}", nsArgErrorCode(error));
    }

    return res;
  }

  return NS_FAILURE;
}

void nsOSFile::InternalClose()
{
  CloseHandle(m_FileData.m_pFileHandle);
  m_FileData.m_pFileHandle = INVALID_HANDLE_VALUE;
}

nsResult nsOSFile::InternalWrite(const void* pBuffer, nsUInt64 uiBytes)
{
  const nsUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBatchBytes))
      return NS_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = nsMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const nsUInt32 uiBytes32 = static_cast<nsUInt32>(uiBytes);

    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBytes32))
      return NS_FAILURE;
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
    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;

    if (uiBytesReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = nsMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const nsUInt32 uiBytes32 = static_cast<nsUInt32>(uiBytes);

    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;
  }

  return uiBytesRead;
}

nsUInt64 nsOSFile::InternalGetFilePosition() const
{
  long int uiHigh32 = 0;
  nsUInt32 uiLow32 = SetFilePointer(m_FileData.m_pFileHandle, 0, &uiHigh32, FILE_CURRENT);

  return nsMath::MakeUInt64(uiHigh32, uiLow32);
}

void nsOSFile::InternalSetFilePosition(nsInt64 iDistance, nsFileSeekMode::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
    case nsFileSeekMode::FromStart:
      NS_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
      break;
    case nsFileSeekMode::FromEnd:
      NS_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
      break;
    case nsFileSeekMode::FromCurrent:
      NS_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
      break;
  }
}

bool nsOSFile::InternalExistsFile(nsStringView sFile)
{
  const DWORD dwAttrib = GetFileAttributesW(nsDosDevicePath(sFile).GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

bool nsOSFile::InternalExistsDirectory(nsStringView sDirectory)
{
  const DWORD dwAttrib = GetFileAttributesW(nsDosDevicePath(sDirectory));

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

nsResult nsOSFile::InternalDeleteFile(nsStringView sFile)
{
  if (DeleteFileW(nsDosDevicePath(sFile)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return NS_SUCCESS;

    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsOSFile::InternalDeleteDirectory(nsStringView sDirectory)
{
  if (RemoveDirectoryW(nsDosDevicePath(sDirectory)) == FALSE)
  {
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
      return NS_SUCCESS;

    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsOSFile::InternalCreateDirectory(nsStringView sDirectory)
{
  // handle drive letters as always successful
  if (nsStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 3) // 'C:\'
    return NS_SUCCESS;

  if (CreateDirectoryW(nsDosDevicePath(sDirectory), nullptr) == FALSE)
  {
    const DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS)
      return NS_SUCCESS;

    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsOSFile::InternalMoveFileOrDirectory(nsStringView sDirectoryFrom, nsStringView sDirectoryTo)
{
  if (MoveFileW(nsDosDevicePath(sDirectoryFrom), nsDosDevicePath(sDirectoryTo)) == 0)
  {
    return NS_FAILURE;
  }
  return NS_SUCCESS;
}

nsString nsOSFile::GetUserDataFolder(nsStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDataPath = nsStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  nsStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

nsString nsOSFile::GetTempDataFolder(nsStringView sSubFolder /*= nullptr*/)
{
  nsStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s = nsStringWChar(pPath);
      s.AppendPath("Temp");
      s_sTempDataPath = s;
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

nsString nsOSFile::GetUserDocumentsFolder(nsStringView sSubFolder /*= {}*/)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_PublicDocuments, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDocumentsPath = nsStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
  }

  nsStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif
