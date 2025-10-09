#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsPhysicsWorldModuleInterface, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_BITFLAGS(nsPhysicsShapeType, 1)
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Static),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Dynamic),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Query),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Trigger),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Character),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Ragdoll),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Rope),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Cloth),
  NS_BITFLAGS_CONSTANT(nsPhysicsShapeType::Debris),
NS_END_STATIC_REFLECTED_BITFLAGS;

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgPhysicsAddImpulse);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgPhysicsAddImpulse, 1, nsRTTIDefaultAllocator<nsMsgPhysicsAddImpulse>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    NS_MEMBER_PROPERTY("Impulse", m_vImpulse),
    NS_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgPhysicCharacterContact);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgPhysicCharacterContact, 1, nsRTTIDefaultAllocator<nsMsgPhysicCharacterContact>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("Character", m_hCharacter),
    NS_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    NS_MEMBER_PROPERTY("Normal", m_vNormal),
    NS_MEMBER_PROPERTY("CharacterVelocity", m_vCharacterVelocity),
    NS_MEMBER_PROPERTY("Impact", m_fImpact),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgPhysicContact);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgPhysicContact, 1, nsRTTIDefaultAllocator<nsMsgPhysicContact>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    NS_MEMBER_PROPERTY("Normal", m_vNormal),
    NS_MEMBER_PROPERTY("ImpactSqr", m_fImpactSqr),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgPhysicsJointBroke);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgPhysicsJointBroke, 1, nsRTTIDefaultAllocator<nsMsgPhysicsJointBroke>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgObjectGrabbed);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgObjectGrabbed, 1, nsRTTIDefaultAllocator<nsMsgObjectGrabbed>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    NS_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgReleaseObjectGrab);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgReleaseObjectGrab, 1, nsRTTIDefaultAllocator<nsMsgReleaseObjectGrab>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

NS_IMPLEMENT_MESSAGE_TYPE(nsMsgBuildStaticMesh);
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgBuildStaticMesh, 1, nsRTTIDefaultAllocator<nsMsgBuildStaticMesh>)
{
  NS_BEGIN_ATTRIBUTES
  {
    new nsExcludeFromScript()
  }
  NS_END_ATTRIBUTES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


NS_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
