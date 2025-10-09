#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class nsWorld;
class nsGameObject;

/// Script extension class providing prefab instantiation functionality for scripts.
class NS_CORE_DLL nsScriptExtensionClass_Prefabs
{
public:
  /// Spawns a prefab instance at the specified global transform.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param globalTransform World position, rotation and scale for the prefab
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static nsVariantArray SpawnPrefab(nsWorld* pWorld, nsStringView sPrefab, const nsTransform& globalTransform, nsUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);

  /// Spawns a prefab instance as a child of the specified parent object.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param pParent Parent game object for the spawned prefab
  /// \param localTransform Local transform relative to the parent
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static nsVariantArray SpawnPrefabAsChild(nsWorld* pWorld, nsStringView sPrefab, nsGameObject* pParent, const nsTransform& localTransform, nsUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_Prefabs);
