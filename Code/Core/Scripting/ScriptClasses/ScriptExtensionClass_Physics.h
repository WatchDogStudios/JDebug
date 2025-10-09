#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class nsWorld;
class nsGameObject;

/// Script extension class providing physics world queries and utilities for scripts.
///
/// Exposes physics system functionality to scripts including collision detection,
/// raycasting, and shape overlap testing. All functions require a valid world
/// and may return no results if no physics world module is active.
class NS_CORE_DLL nsScriptExtensionClass_Physics
{
public:
  /// Gets the current gravity vector for the physics world.
  static nsVec3 GetGravity(nsWorld* pWorld);

  /// Finds collision layer index by name, returns invalid index if not found.
  static nsUInt8 GetCollisionLayerByName(nsWorld* pWorld, nsStringView sLayerName);

  /// Finds weight category index by name, returns invalid key if not found.
  static nsUInt8 GetWeightCategoryByName(nsWorld* pWorld, nsStringView sCategoryName);

  /// Finds impulse type index by name, returns invalid key if not found.
  static nsUInt8 GetImpulseTypeByName(nsWorld* pWorld, nsStringView sImpulseTypeName);

  /// Performs raycast and returns hit information if collision is found.
  static bool Raycast(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, const nsVec3& vStart, const nsVec3& vDirection, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic, nsUInt32 uiIgnoreObjectID = nsInvalidIndex);

  /// Tests if a line segment intersects with any physics shapes.
  static bool OverlapTestLine(nsWorld* pWorld, const nsVec3& vStart, const nsVec3& vEnd, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic, nsUInt32 uiIgnoreObjectID = nsInvalidIndex);

  /// Tests if a sphere at the given position overlaps with any physics shapes.
  static bool OverlapTestSphere(nsWorld* pWorld, float fRadius, const nsVec3& vPosition, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic);

  /// Tests if a capsule with the given transform overlaps with any physics shapes.
  static bool OverlapTestCapsule(nsWorld* pWorld, float fRadius, float fHeight, const nsTransform& transform, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic);

  /// Sweeps a sphere along a direction and returns hit information if collision is found.
  static bool SweepTestSphere(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, float fRadius, const nsVec3& vStart, const nsVec3& vDirection, float fDistance, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic);

  /// Sweeps a capsule along a direction and returns hit information if collision is found.
  static bool SweepTestCapsule(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, float fRadius, float fHeight, const nsTransform& start, const nsVec3& vDirection, float fDistance, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic);

  /// Performs raycast and triggers surface interaction at hit point if collision is found.
  static bool RaycastSurfaceInteraction(nsWorld* pWorld, const nsVec3& vRayStart, const nsVec3& vRayDirection, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes, nsStringView sFallbackSurface, const nsTempHashedString& sInteraction, float fInteractionImpulse, nsUInt32 uiIgnoreObjectID = nsInvalidIndex);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_Physics);
