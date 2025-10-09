#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct NS_TOOLSFOUNDATION_DLL nsToolsTag
{
  nsToolsTag() = default;
  nsToolsTag(nsStringView sCategory, nsStringView sName, bool bBuiltIn = false)
    : m_sCategory(sCategory)
    , m_sName(sName)
    , m_bBuiltInTag(bBuiltIn)
  {
  }

  nsString m_sCategory;
  nsString m_sName;
  bool m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class NS_TOOLSFOUNDATION_DLL nsToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'.
  static void Clear();

  /// \brief Serializes all tags to a DDL stream.
  static void WriteToDDL(nsStreamWriter& inout_stream);
  /// \brief Reads tags from a DDL stream.
  static nsStatus ReadFromDDL(nsStreamReader& inout_stream);

  /// \brief Adds a tag to the registry. Returns true if the tag was valid.
  static bool AddTag(const nsToolsTag& tag);
  /// \brief Removes a tag by name. Returns true if the tag was removed.
  static bool RemoveTag(nsStringView sName);

  /// \brief Retrieves all tags in the registry.
  static void GetAllTags(nsHybridArray<const nsToolsTag*, 16>& out_tags);
  /// \brief Retrieves all tags in the given categories.
  static void GetTagsByCategory(const nsArrayPtr<nsStringView>& categories, nsHybridArray<const nsToolsTag*, 16>& out_tags);

private:
  static nsMap<nsString, nsToolsTag> s_NameToTags;
};
