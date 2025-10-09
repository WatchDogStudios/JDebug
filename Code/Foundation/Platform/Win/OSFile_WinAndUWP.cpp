#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <Foundation/Threading/ThreadUtils.h>

// Defined in Timestamp_Win.cpp
nsInt64 FileTimeToEpoch(FILETIME fileTime);

nsResult nsOSFile::InternalGetFileStats(nsStringView sFileOrFolder, nsFileStats& out_Stats)
{
  nsStringBuilder s = sFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  s.Trim(nullptr, "/\\");

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sParentPath.Clear();
    out_Stats.m_sName = s;
    out_Stats.m_LastModificationTime = nsTimestamp::MakeInvalid();
    return NS_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(nsDosDevicePath(s), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return NS_FAILURE;

  out_Stats.m_uiFileSize = nsMath::MakeUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = data.cFileName;
  out_Stats.m_LastModificationTime = nsTimestamp::MakeFromInt(FileTimeToEpoch(data.ftLastWriteTime), nsSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return NS_SUCCESS;
}

nsStringView nsOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    nsUInt32 uiRequiredLength = 512;
    nsHybridArray<wchar_t, 1024> tmp;

    while (true)
    {
      tmp.SetCountUninitialized(uiRequiredLength);

      // reset last error code
      SetLastError(ERROR_SUCCESS);

      const nsUInt32 uiLength = GetModuleFileNameW(nullptr, tmp.GetData(), tmp.GetCount() - 1);
      const DWORD error = GetLastError();

      if (error == ERROR_SUCCESS)
      {
        tmp[uiLength] = L'\0';
        break;
      }

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
        uiRequiredLength += 512;
        continue;
      }

      NS_REPORT_FAILURE("GetModuleFileNameW failed: {0}", nsArgErrorCode(error));
    }

    s_sApplicationPath = nsStringUtf8(tmp.GetData()).GetData();
  }

  return s_sApplicationPath;
}

const nsString nsOSFile::GetCurrentWorkingDirectory()
{
  const nsUInt32 uiRequiredLength = GetCurrentDirectoryW(0, nullptr);

  nsHybridArray<wchar_t, 1024> tmp;
  tmp.SetCountUninitialized(uiRequiredLength + 16);

  if (GetCurrentDirectoryW(tmp.GetCount() - 1, tmp.GetData()) == 0)
  {
    NS_REPORT_FAILURE("GetCurrentDirectoryW failed: {}", nsArgErrorCode(GetLastError()));
    return nsString();
  }

  tmp[uiRequiredLength] = L'\0';

  nsStringBuilder clean = nsStringUtf8(tmp.GetData()).GetData();
  clean.MakeCleanPath();

  return clean;
}

#endif
