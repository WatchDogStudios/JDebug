#pragma once

#include <Core/Collection/CollectionResource.h>

class nsHashedString;

namespace nsCollectionUtils
{
  /// \brief Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see nsResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  NS_CORE_DLL void AddFiles(nsCollectionResourceDescriptor& ref_collection, nsStringView sAssetTypeName, nsStringView sAbsPathToFolder,
    nsStringView sFileExtension, nsStringView sStripPrefix, nsStringView sPrependPrefix);

  /// \brief Merges all collections from the input array into the target result collection. Resource entries will be de-duplicated by resource ID
  /// string.
  NS_CORE_DLL void MergeCollections(nsCollectionResourceDescriptor& ref_result, nsArrayPtr<const nsCollectionResourceDescriptor*> inputCollections);

  /// \brief Special case of nsCollectionUtils::MergeCollections which outputs unique entries from input collection into the result collection
  NS_CORE_DLL void DeDuplicateEntries(nsCollectionResourceDescriptor& ref_result, const nsCollectionResourceDescriptor& input);

  /// \brief Extracts info (i.e. resource ID as file path) from the passed handle and adds it as a new resource entry. Does not add an entry if the
  /// resource handle is not valid.
  ///
  /// The resource type identifier must be passed explicity as szAssetTypeName (see nsResourceManager::RegisterResourceForAssetType). To determine the
  /// file size, the resource ID is used as a filename passed to nsFileSystem::GetFileStats. In case the resource's path root is not mounted, the path
  /// root can be replaced by passing non-NULL string to szAbsFolderpath, which will replace the root, e.g. with an absolute file path. This is just
  /// for the file size check within the scope of the function, it will not modify the resource Id.
  NS_CORE_DLL void AddResourceHandle(nsCollectionResourceDescriptor& ref_collection, nsTypelessResourceHandle hHandle, nsStringView sAssetTypeName, nsStringView sAbsFolderpath);

}; // namespace nsCollectionUtils
