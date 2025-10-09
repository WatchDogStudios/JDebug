#pragma once

/// \file

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Strings/HashedString.h>

class nsRTTI;
class nsAbstractObjectNode;
class nsAbstractObjectGraph;
class nsGraphPatch;
class nsGraphPatchContext;
class nsGraphVersioning;

/// \brief Identifier for graph patches combining type name and version number.
///
/// This structure uniquely identifies which patch should be applied to which type version.
/// The versioning system uses this to track patch progression and avoid duplicate applications.
struct nsVersionKey
{
  nsVersionKey() = default;
  nsVersionKey(nsStringView sType, nsUInt32 uiTypeVersion)
  {
    m_sType.Assign(sType);
    m_uiTypeVersion = uiTypeVersion;
  }
  NS_DECLARE_POD_TYPE();
  nsHashedString m_sType;
  nsUInt32 m_uiTypeVersion;
};

/// \brief Hash helper class for nsVersionKey
struct nsGraphVersioningHash
{
  NS_FORCE_INLINE static nsUInt32 Hash(const nsVersionKey& a)
  {
    auto typeNameHash = a.m_sType.GetHash();
    nsUInt32 uiHash = nsHashingUtils::xxHash32(&typeNameHash, sizeof(typeNameHash));
    uiHash = nsHashingUtils::xxHash32(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  NS_ALWAYS_INLINE static bool Equal(const nsVersionKey& a, const nsVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// \brief Stores type version information required for graph patching operations.
///
/// This structure contains the metadata needed to apply version patches, including
/// type names and version numbers for both the type and its parent class hierarchy.
/// It overlaps with nsReflectedTypeDescriptor to enable efficient patch processing.
struct NS_FOUNDATION_DLL nsTypeVersionInfo
{
  const char* GetTypeName() const;
  void SetTypeName(const char* szName);
  const char* GetParentTypeName() const;
  void SetParentTypeName(const char* szName);

  nsHashedString m_sTypeName;
  nsHashedString m_sParentTypeName;
  nsUInt32 m_uiTypeVersion;
};
NS_DECLARE_REFLECTABLE_TYPE(NS_FOUNDATION_DLL, nsTypeVersionInfo);

/// \brief Context object that manages the patching process for individual nodes.
///
/// This class is passed to patch implementations to provide utility functions and track
/// the patching progress of a node. It handles base class patching, type renaming, and
/// hierarchy changes while maintaining consistency across the entire patching process.
class NS_FOUNDATION_DLL nsGraphPatchContext
{
public:
  /// \brief Ensures a base class is patched to the specified version before continuing.
  ///
  /// This function forces the base class to be at the specified version, applying patches if necessary.
  /// Use bForcePatch for backwards compatibility when base class type information wasn't originally
  /// serialized. This ensures proper patch ordering in inheritance hierarchies.
  void PatchBaseClass(const char* szType, nsUInt32 uiTypeVersion, bool bForcePatch = false); // [tested]

  /// \brief Renames the current node's type to a new type name.
  ///
  /// Used when types are renamed or moved to different namespaces. The version number
  /// is preserved unless explicitly changed with the overload that takes a version parameter.
  void RenameClass(const char* szTypeName); // [tested]

  /// \brief Renames the current node's type and sets a new version number.
  ///
  /// Use this when both the type name and version change during a patch operation.
  /// This is common when types are refactored or split into multiple classes.
  void RenameClass(const char* szTypeName, nsUInt32 uiVersion);

  /// \brief Replaces the entire base class hierarchy with a new one.
  ///
  /// This is used for major refactoring where the inheritance structure changes.
  /// The array should contain the complete new inheritance chain from most derived
  /// to most base class. Handle with care as this affects serialization compatibility.
  void ChangeBaseClass(nsArrayPtr<nsVersionKey> baseClasses); // [tested]

private:
  friend class nsGraphVersioning;
  nsGraphPatchContext(nsGraphVersioning* pParent, nsAbstractObjectGraph* pGraph, nsAbstractObjectGraph* pTypesGraph);
  void Patch(nsAbstractObjectNode* pNode);
  void Patch(nsUInt32 uiBaseClassIndex, nsUInt32 uiTypeVersion, bool bForcePatch);
  void UpdateBaseClasses();

private:
  nsGraphVersioning* m_pParent = nullptr;
  nsAbstractObjectGraph* m_pGraph = nullptr;
  nsAbstractObjectNode* m_pNode = nullptr;
  nsDynamicArray<nsVersionKey> m_BaseClasses;
  nsUInt32 m_uiBaseClassIndex = 0;
  mutable nsHashTable<nsHashedString, nsTypeVersionInfo> m_TypeToInfo;
};

/// \brief Singleton system that manages version patching for nsAbstractObjectGraph instances.
///
/// This system automatically applies version patches during deserialization to handle data migration
/// when type definitions change between versions. It supports both node-level patches (specific type
/// transformations) and graph-level patches (global transformations affecting multiple types).
///
/// The system automatically executes during nsAbstractObjectGraph deserialization,
/// ensuring that older serialized data can be loaded into newer application versions.
class NS_FOUNDATION_DLL nsGraphVersioning
{
  NS_DECLARE_SINGLETON(nsGraphVersioning);

public:
  nsGraphVersioning();
  ~nsGraphVersioning();

  /// \brief Applies all necessary patches to bring the graph to the current version.
  ///
  /// This is the main entry point for graph patching. It processes all nodes in the graph,
  /// applying patches in dependency order to ensure consistency.
  ///
  /// \param pGraph The object graph to patch (modified in-place)
  /// \param pTypesGraph Optional type information graph from serialization time.
  ///        Contains the exact type versions that were serialized. If not provided,
  ///        base classes are assumed to be at their maximum patchable version.
  ///
  /// The patching process:
  /// 1. Discovers all required patches for each node type
  /// 2. Sorts patches by dependency order (base classes first)
  /// 3. Applies patches incrementally until all nodes reach current versions
  /// 4. Validates that no circular dependencies exist
  void PatchGraph(nsAbstractObjectGraph* pGraph, nsAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class nsGraphPatchContext;

  NS_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void PluginEventHandler(const nsPluginEvent& EventData);
  void UpdatePatches();
  nsUInt32 GetMaxPatchVersion(const nsHashedString& sType) const;

  nsHashTable<nsHashedString, nsUInt32> m_MaxPatchVersion; ///< Max version the given type can be patched to.
  nsDynamicArray<const nsGraphPatch*> m_GraphPatches;
  nsHashTable<nsVersionKey, const nsGraphPatch*, nsGraphVersioningHash> m_NodePatches;
};
