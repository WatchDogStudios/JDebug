#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>

struct nsGameObjectHandle;
struct nsSkeletonResourceDescriptor;

/// Interface for physics world modules that provide physics simulation and queries.
///
/// Physics world modules implement physics functionality for a world, including
/// collision detection, raycasting, and shape queries. Different physics engines
/// can provide their own implementations of this interface.
class NS_CORE_DLL nsPhysicsWorldModuleInterface : public nsWorldModule
{
  NS_ADD_DYNAMIC_REFLECTION(nsPhysicsWorldModuleInterface, nsWorldModule);

protected:
  nsPhysicsWorldModuleInterface(nsWorld* pWorld)
    : nsWorldModule(pWorld)
  {
  }

public:
  /// \brief Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns nsInvalidIndex if no such collision layer exists.
  virtual nsUInt32 GetCollisionLayerByName(nsStringView sName) const = 0;

  /// \brief Searches for a weight category with the given name and returns its key.
  ///
  /// Returns nsWeightCategoryConfig::InvalidKey if no such category exists.
  virtual nsUInt8 GetWeightCategoryByName(nsStringView sName) const = 0;

  /// \brief Searches for an impulse type with the given name and returns its key.
  ///
  /// Returns nsImpulseTypeConfig::InvalidKey if no such category exists.
  virtual nsUInt8 GetImpulseTypeByName(nsStringView sName) const = 0;

  virtual bool Raycast(nsPhysicsCastResult& out_result, const nsVec3& vStart, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params, nsPhysicsHitCollection collection = nsPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(nsPhysicsCastResultArray& out_results, const nsVec3& vStart, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(nsPhysicsCastResult& out_result, float fSphereRadius, const nsVec3& vStart, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params, nsPhysicsHitCollection collection = nsPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(nsPhysicsCastResult& out_result, const nsVec3& vBoxExtents, const nsTransform& transform, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params, nsPhysicsHitCollection collection = nsPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(nsPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const nsTransform& transform, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params, nsPhysicsHitCollection collection = nsPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCylinder(nsPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const nsTransform& transform, const nsVec3& vDir, float fDistance, const nsPhysicsQueryParameters& params, nsPhysicsHitCollection collection = nsPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const nsVec3& vPosition, const nsPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestBox(const nsVec3& vBoxExtents, const nsVec3& vPosition, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(nsPhysicsOverlapResultArray& out_results, float fSphereRadius, const nsVec3& vPosition, const nsPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInBox(nsPhysicsOverlapResultArray& out_results, const nsVec3& vBoxExtents, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCapsule(nsPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInCylinder(nsPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const nsTransform& transform, const nsPhysicsQueryParameters& params) const = 0;

  virtual nsVec3 GetGravity() const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// \brief Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(nsGameObject* pOwner, nsVec3 vBoxSize)
  {
    NS_IGNORE_UNUSED(pOwner);
    NS_IGNORE_UNUSED(vBoxSize);
  }

  struct JointConfig
  {
    nsGameObjectHandle m_hActorA;
    nsGameObjectHandle m_hActorB;
    nsTransform m_LocalFrameA = nsTransform::MakeIdentity();
    nsTransform m_LocalFrameB = nsTransform::MakeIdentity();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// \brief Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(nsGameObject* pOwner, const nsPhysicsWorldModuleInterface::FixedJointConfig& cfg)
  {
    NS_IGNORE_UNUSED(pOwner);
    NS_IGNORE_UNUSED(cfg);
  }

  /// \brief Gets world space bounds of a physics object if its shape type is included in shapeTypes and its collision layer interacts with uiCollisionLayer.
  virtual nsBoundingBoxSphere GetWorldSpaceBounds(nsGameObject* pOwner, nsUInt32 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
  {
    NS_IGNORE_UNUSED(pOwner);
    NS_IGNORE_UNUSED(uiCollisionLayer);
    NS_IGNORE_UNUSED(shapeTypes);
    NS_IGNORE_UNUSED(bIncludeChildObjects);
    return nsBoundingBoxSphere::MakeInvalid();
  }
};

/// \brief Used to apply a physical impulse on the object
struct NS_CORE_DLL nsMsgPhysicsAddImpulse : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgPhysicsAddImpulse, nsMessage);

  nsVec3 m_vGlobalPosition;
  nsVec3 m_vImpulse;
  nsUInt8 m_uiImpulseType = 0;
  nsUInt32 m_uiObjectFilterID = nsInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct NS_CORE_DLL nsMsgPhysicsJointBroke : public nsEventMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgPhysicsJointBroke, nsEventMessage);

  nsGameObjectHandle m_hJointObject;
};

/// \brief Sent by components such as nsJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct NS_CORE_DLL nsMsgObjectGrabbed : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgObjectGrabbed, nsMessage);

  nsGameObjectHandle m_hGrabbedBy;
  bool m_bGotGrabbed = true;
};

/// \brief Send this to components such as nsJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct NS_CORE_DLL nsMsgReleaseObjectGrab : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgReleaseObjectGrab, nsMessage);

  nsGameObjectHandle m_hGrabbedObjectToRelease;
};

/// \brief Can be sent by character controllers to inform objects when a CC pushes into them.
///
/// Whether this message is sent, depends on the character controller implementation.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct NS_CORE_DLL nsMsgPhysicCharacterContact : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgPhysicCharacterContact, nsMessage);

  nsComponentHandle m_hCharacter;
  nsVec3 m_vGlobalPosition;
  nsVec3 m_vNormal;
  nsVec3 m_vCharacterVelocity;
  float m_fImpact;
};

/// \brief Sent to physics components that have contact reporting enabled (see nsOnJoltContact::SendContactMsg).
///
/// Only sent for certain physics object combinations, e.g. debris doesn't trigger this.
/// The reported contact position and normal is an average of the contact manifold.
/// This is mainly meant for less important interactions, like breaking decorative things.
struct NS_CORE_DLL nsMsgPhysicContact : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgPhysicContact, nsMessage);

  nsVec3 m_vGlobalPosition;
  nsVec3 m_vNormal;
  float m_fImpactSqr;
};


//////////////////////////////////////////////////////////////////////////

struct NS_CORE_DLL nsSmcTriangle
{
  NS_DECLARE_POD_TYPE();

  nsUInt32 m_uiVertexIndices[3];
};

struct NS_CORE_DLL nsSmcSubMesh
{
  NS_DECLARE_POD_TYPE();

  nsUInt32 m_uiFirstTriangle = 0;
  nsUInt32 m_uiNumTriangles = 0;
  nsUInt16 m_uiSurfaceIndex = 0;
};

struct NS_CORE_DLL nsSmcDescription
{
  nsDeque<nsVec3> m_Vertices;
  nsDeque<nsSmcTriangle> m_Triangles;
  nsDeque<nsSmcSubMesh> m_SubMeshes;
  nsDeque<nsString> m_Surfaces;
};

struct NS_CORE_DLL nsMsgBuildStaticMesh : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgBuildStaticMesh, nsMessage);

  /// \brief Append data to this description to add meshes to the automatic static mesh generation
  nsSmcDescription* m_pStaticMeshDescription = nullptr;
};
