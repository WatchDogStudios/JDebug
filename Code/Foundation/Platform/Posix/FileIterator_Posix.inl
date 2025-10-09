
#if NS_DISABLED(NS_SUPPORTS_FILE_ITERATORS)
#  error "Don't include this file on platforms that don't support file iterators."
#endif

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include <dirent.h>
#include <fnmatch.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

nsFileSystemIterator::nsFileSystemIterator() = default;

nsFileSystemIterator::~nsFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool nsFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

namespace
{
  nsResult UpdateCurrentFile(nsFileStats& curFile, const nsStringBuilder& curPath, DIR* hSearch, const nsString& wildcardSearch)
  {
    struct dirent* hCurrentFile = readdir(hSearch);
    if (hCurrentFile == nullptr)
      return NS_FAILURE;

    if (!wildcardSearch.IsEmpty())
    {
      while (fnmatch(wildcardSearch.GetData(), hCurrentFile->d_name, FNM_NOESCAPE) != 0)
      {
        hCurrentFile = readdir(hSearch);
        if (hCurrentFile == nullptr)
          return NS_FAILURE;
      }
    }

    nsStringBuilder absFileName = curPath;
    absFileName.AppendPath(hCurrentFile->d_name);

    struct stat fileStat = {};
    stat(absFileName.GetData(), &fileStat);

    curFile.m_uiFileSize = fileStat.st_size;
    curFile.m_bIsDirectory = hCurrentFile->d_type == DT_DIR;
    curFile.m_sParentPath = curPath;
    curFile.m_sName = hCurrentFile->d_name;
#ifdef __USE_XOPEN2K8
    curFile.m_LastModificationTime = nsTimestamp::MakeFromInt(fileStat.st_mtim.tv_sec * 1000000000ull + fileStat.st_mtim.tv_nsec, nsSIUnitOfTime::Nanosecond);
#else
    curFile.m_LastModificationTime = nsTimestamp::MakeFromInt(fileStat.st_mtime, nsSIUnitOfTime::Second);
#endif

    return NS_SUCCESS;
  }
} // namespace

void nsFileSystemIterator::StartSearch(nsStringView sSearchTerm, nsBitflags<nsFileSystemIteratorFlags> flags /*= nsFileSystemIteratorFlags::All*/)
{
  NS_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchTerm;

  nsStringBuilder sSearch = sSearchTerm;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // Remove a trailing slash if any
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  if (flags.IsSet(nsFileSystemIteratorFlags::Recursive) == true && bHasWildcard == true)
  {
    NS_ASSERT_DEV(false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");
    return;
  }

  if (bHasWildcard)
  {
    m_Data.m_wildcardSearch = sSearch.GetFileNameAndExtension();
    m_sCurPath = sSearch.GetFileDirectory();
  }
  else
  {
    m_Data.m_wildcardSearch.Clear();
    m_sCurPath = sSearch;
  }

  NS_ASSERT_DEV(m_sCurPath.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  DIR* hSearch = opendir(m_sCurPath.GetData());

  if (hSearch == nullptr)
    return;

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Failed())
  {
    return;
  }

  m_Data.m_Handles.PushBack(hSearch);

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
  {
    Next(); // will search for the next file or folder that is not ".." or "." ; might return false though
    return;
  }

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFolders))
    {
      Next();
      return;
    }
  }
  else
  {
    if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFiles))
    {
      Next();
      return;
    }
  }
}

nsInt32 nsFileSystemIterator::InternalNext()
{
  constexpr nsInt32 CallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return NS_FAILURE;

  if (m_Flags.IsSet(nsFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName.GetData());

    DIR* hSearch = opendir(m_sCurPath.GetData());

    if (hSearch != nullptr && UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Succeeded())
    {
      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return CallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFolders))
          return CallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFiles))
          return CallInternalNext;
      }

      return NS_SUCCESS;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, (DIR*)m_Data.m_Handles.PeekBack(), m_Data.m_wildcardSearch).Failed())
  {
    // nothing found in this directory anymore
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return NS_FAILURE;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.GetElementCount() > 1 && m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1);
    }

    return CallInternalNext;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return CallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFolders))
      return CallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(nsFileSystemIteratorFlags::ReportFiles))
      return CallInternalNext;
  }

  return NS_SUCCESS;
}
