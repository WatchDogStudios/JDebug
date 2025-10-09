#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>


using nsSurfaceResourceHandle = nsTypedResourceHandle<class nsSurfaceResource>;

/// \brief Classifies the facing of an individual raycast hit
enum class nsPhysicsHitType : int8_t
{
  Undefined = -1,        ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0, ///< The raycast hit the front face of a triangle
  TriangleBackFace = 1,  ///< The raycast hit the back face of a triangle
};

/// \brief Used for raycast and sweep tests
struct nsPhysicsCastResult
{
  nsVec3 m_vPosition;
  nsVec3 m_vNormal;
  float m_fDistance;

  nsGameObjectHandle m_hShapeObject;                        ///< The game object to which the hit physics shape is attached.
  nsGameObjectHandle m_hActorObject;                        ///< The game object to which the parent actor of the hit physics shape is attached.
  nsSurfaceResourceHandle m_hSurface;                       ///< The type of surface that was hit (if available)
  nsUInt32 m_uiObjectFilterID = nsInvalidIndex;             ///< An ID either per object (rigid-body / ragdoll) or per shape (implementation specific) that can be used to ignore this object during raycasts and shape queries.
  nsPhysicsHitType m_hitType = nsPhysicsHitType::Undefined; ///< Classification of the triangle face, see nsPhysicsHitType

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct nsPhysicsCastResultArray
{
  nsHybridArray<nsPhysicsCastResult, 16> m_Results;
};

/// \brief Used to report overlap query results
struct nsPhysicsOverlapResult
{
  NS_DECLARE_POD_TYPE();

  nsGameObjectHandle m_hShapeObject;            ///< The game object to which the hit physics shape is attached.
  nsGameObjectHandle m_hActorObject;            ///< The game object to which the parent actor of the hit physics shape is attached.
  nsUInt32 m_uiObjectFilterID = nsInvalidIndex; ///< The shape id of the hit physics shape
  nsVec3 m_vCenterPosition;                     ///< The center position of the reported object in world space.

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct nsPhysicsOverlapResultArray
{
  nsHybridArray<nsPhysicsOverlapResult, 16> m_Results;
};

/// \brief Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
///
/// This is mainly for optimization purposes. It is up to the physics integration to support some or all of these flags.
///
/// Note: If this is modified, 'Physics.ts' also has to be updated.
NS_DECLARE_FLAGS_WITH_DEFAULT(nsUInt32, nsPhysicsShapeType, 0xFFFFFFFF,
  Static,    ///< Static geometry
  Dynamic,   ///< Dynamic and kinematic objects
  Query,     ///< Query shapes are kinematic bodies that don't participate in the simulation and are only used for raycasts and other queries.
  Trigger,   ///< Trigger shapes
  Character, ///< Shapes associated with character controllers.
  Ragdoll,   ///< All shapes belonging to ragdolls.
  Rope,      ///< All shapes belonging to ropes.
  Cloth,     ///< Soft-body shapes. Mainly for decorative purposes.
  Debris     ///< Small stuff for visuals, but shouldn't affect the game. This will only have one-way interactions, ie get pushed, but won't push others.
);

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsPhysicsShapeType);

struct nsPhysicsQueryParameters
{
  nsPhysicsQueryParameters() = default;
  explicit nsPhysicsQueryParameters(nsUInt32 uiCollisionLayer,
    nsBitflags<nsPhysicsShapeType> shapeTypes = nsPhysicsShapeType::Default, nsUInt32 uiIgnoreObjectFilterID = nsInvalidIndex)
    : m_uiCollisionLayer(uiCollisionLayer)
    , m_ShapeTypes(shapeTypes)
    , m_uiIgnoreObjectFilterID(uiIgnoreObjectFilterID)
  {
  }

  nsUInt32 m_uiCollisionLayer = 0;
  nsBitflags<nsPhysicsShapeType> m_ShapeTypes = nsPhysicsShapeType::Default;
  nsUInt32 m_uiIgnoreObjectFilterID = nsInvalidIndex;
  bool m_bIgnoreInitialOverlap = false;
};

enum class nsPhysicsHitCollection
{
  Closest,
  Any
};
