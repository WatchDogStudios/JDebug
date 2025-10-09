#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void nsCollectionUtils::AddFiles(nsCollectionResourceDescriptor& ref_collection, nsStringView sAssetTypeNameView, nsStringView sAbsPathToFolder, nsStringView sFileExtension, nsStringView sStripPrefix, nsStringView sPrependPrefix)
{
#if NS_ENABLED(NS_SUPPORTS_FILE_ITERATORS)

  const nsUInt32 uiStripPrefixLength = nsStringUtils::GetCharacterCount(sStripPrefix.GetStartPointer(), sStripPrefix.GetEndPointer());

  nsFileSystemIterator fsIt;
  fsIt.StartSearch(sAbsPathToFolder, nsFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  nsStringBuilder sFullPath;
  nsHashedString sAssetTypeName;
  sAssetTypeName.Assign(sAssetTypeNameView);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (nsPathUtils::HasExtension(stats.m_sName, sFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(sPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry = ref_collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID = sFullPath;
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }

#else
  NS_IGNORE_UNUSED(ref_collection);
  NS_IGNORE_UNUSED(sAssetTypeNameView);
  NS_IGNORE_UNUSED(sAbsPathToFolder);
  NS_IGNORE_UNUSED(sFileExtension);
  NS_IGNORE_UNUSED(sStripPrefix);
  NS_IGNORE_UNUSED(sPrependPrefix);
  NS_ASSERT_NOT_IMPLEMENTED;
#endif
}


NS_CORE_DLL void nsCollectionUtils::MergeCollections(nsCollectionResourceDescriptor& ref_result, nsArrayPtr<const nsCollectionResourceDescriptor*> inputCollections)
{
  nsMap<nsString, const nsCollectionEntry*> firstEntryOfID;

  for (const nsCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const nsCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        ref_result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


NS_CORE_DLL void nsCollectionUtils::DeDuplicateEntries(nsCollectionResourceDescriptor& ref_result, const nsCollectionResourceDescriptor& input)
{
  const nsCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(ref_result, nsArrayPtr<const nsCollectionResourceDescriptor*>(&firstInput, 1));
}

void nsCollectionUtils::AddResourceHandle(nsCollectionResourceDescriptor& ref_collection, nsTypelessResourceHandle hHandle, nsStringView sAssetTypeName, nsStringView sAbsFolderpath)
{
  if (!hHandle.IsValid())
    return;

  const nsStringView resID = hHandle.GetResourceID();

  auto& entry = ref_collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(sAssetTypeName);
  entry.m_sResourceID = resID;

  nsStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (!sAbsFolderpath.IsEmpty())
  {
    nsStringView root, relFile;
    nsPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = sAbsFolderpath;
    absFilename.AppendPath(relFile);
    absFilename.MakeCleanPath();

    nsFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && nsFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}
