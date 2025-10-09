#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Physics.h>

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_Physics, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(GetGravity, In, "World"),
    NS_SCRIPT_FUNCTION_PROPERTY(GetCollisionLayerByName, In, "World", In, "Name"),
    NS_SCRIPT_FUNCTION_PROPERTY(GetWeightCategoryByName, In, "World", In, "Name"),
    NS_SCRIPT_FUNCTION_PROPERTY(GetImpulseTypeByName, In, "World", In, "Name"),

    NS_SCRIPT_FUNCTION_PROPERTY(Raycast, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Start", In, "Direction", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(
      new nsFunctionArgumentAttributes(6, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(7, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic)),
      new nsFunctionArgumentAttributes(8, new nsDefaultValueAttribute((nsInt32)nsInvalidIndex))),

    NS_SCRIPT_FUNCTION_PROPERTY(OverlapTestLine, In, "World", In, "Start", In, "End", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(
      new nsFunctionArgumentAttributes(3, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(4, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic)),
      new nsFunctionArgumentAttributes(5, new nsDefaultValueAttribute((nsInt32)nsInvalidIndex))),

    NS_SCRIPT_FUNCTION_PROPERTY(OverlapTestSphere, In, "World", In, "Radius", In, "Position", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new nsFunctionArgumentAttributes(3, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(4, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic))),

    NS_SCRIPT_FUNCTION_PROPERTY(OverlapTestCapsule, In, "World", In, "Radius", In, "Height", In, "Transform", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new nsFunctionArgumentAttributes(4, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(5, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic))),

    NS_SCRIPT_FUNCTION_PROPERTY(SweepTestSphere, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new nsFunctionArgumentAttributes(8, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(9, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic))),

    NS_SCRIPT_FUNCTION_PROPERTY(SweepTestCapsule, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Height", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new nsFunctionArgumentAttributes(9, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(10, new nsDefaultValueAttribute((nsInt32)nsPhysicsShapeType::Static | (nsInt32)nsPhysicsShapeType::Dynamic))),

    NS_SCRIPT_FUNCTION_PROPERTY(RaycastSurfaceInteraction, In, "World", In, "RayStart", In, "RayDirection", In, "CollisionLayer", In, "ShapeTypes", In, "FallbackSurface", In, "Interaction", In, "Impulse", In, "IgnoreObjectID")->AddAttributes(
      new nsFunctionArgumentAttributes(3, new nsDynamicEnumAttribute("PhysicsCollisionLayer")),
      new nsFunctionArgumentAttributes(7, new nsDefaultValueAttribute(0.0f)),
      new nsFunctionArgumentAttributes(8, new nsDefaultValueAttribute((nsInt32)nsInvalidIndex))),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("Physics"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on

nsVec3 nsScriptExtensionClass_Physics::GetGravity(nsWorld* pWorld)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    return pModule->GetGravity();
  }

  return nsVec3::MakeZero();
}

nsUInt8 nsScriptExtensionClass_Physics::GetCollisionLayerByName(nsWorld* pWorld, nsStringView sLayerName)
{
  if (nsPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<nsPhysicsWorldModuleInterface>())
  {
    return static_cast<nsUInt8>(pInterface->GetCollisionLayerByName(sLayerName));
  }

  return 0;
}

nsUInt8 nsScriptExtensionClass_Physics::GetWeightCategoryByName(nsWorld* pWorld, nsStringView sCategoryName)
{
  if (nsPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<nsPhysicsWorldModuleInterface>())
  {
    return static_cast<nsUInt8>(pInterface->GetWeightCategoryByName(sCategoryName));
  }

  return 255;
}

nsUInt8 nsScriptExtensionClass_Physics::GetImpulseTypeByName(nsWorld* pWorld, nsStringView sImpulseTypeName)
{
  if (nsPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<nsPhysicsWorldModuleInterface>())
  {
    return static_cast<nsUInt8>(pInterface->GetImpulseTypeByName(sImpulseTypeName));
  }

  return 255;
}

bool nsScriptExtensionClass_Physics::Raycast(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, const nsVec3& vStart, const nsVec3& vDirection, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/, nsUInt32 uiIgnoreObjectID)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsCastResult res;
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    if (pModule->Raycast(res, vStart, vDirection, 1.0f, params))
    {
      // res.m_hSurface
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }

  return false;
}

bool nsScriptExtensionClass_Physics::OverlapTestLine(nsWorld* pWorld, const nsVec3& vStart, const nsVec3& vEnd, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/, nsUInt32 uiIgnoreObjectID /*= nsInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsCastResult res;
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    nsVec3 vDirection = vEnd - vStart;
    const float fDistance = vDirection.GetLengthAndNormalize();

    if (pModule->Raycast(res, vStart, vDirection, fDistance, params))
    {
      return true;
    }
  }

  return false;
}

bool nsScriptExtensionClass_Physics::OverlapTestSphere(nsWorld* pWorld, float fRadius, const nsVec3& vPosition, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestSphere(fRadius, vPosition, params);
  }

  return false;
}

bool nsScriptExtensionClass_Physics::OverlapTestCapsule(nsWorld* pWorld, float fRadius, float fHeight, const nsTransform& transform, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestCapsule(fRadius, fHeight, transform, params);
  }
  return false;
}

bool nsScriptExtensionClass_Physics::SweepTestSphere(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, float fRadius, const nsVec3& vStart, const nsVec3& vDirection, float fDistance, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsCastResult res;
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestSphere(res, fRadius, vStart, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }
  return false;
}

bool nsScriptExtensionClass_Physics::SweepTestCapsule(nsVec3& out_vHitPosition, nsVec3& out_vHitNormal, nsGameObjectHandle& out_hHitObject, nsWorld* pWorld, float fRadius, float fHeight, const nsTransform& start, const nsVec3& vDirection, float fDistance, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes /*= nsPhysicsShapeType::Static | nsPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsCastResult res;
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestCapsule(res, fRadius, fHeight, start, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }
  return false;
}

bool nsScriptExtensionClass_Physics::RaycastSurfaceInteraction(nsWorld* pWorld, const nsVec3& vRayStart, const nsVec3& vRayDirection, nsUInt8 uiCollisionLayer, nsBitflags<nsPhysicsShapeType> shapeTypes, nsStringView sFallbackSurface, const nsTempHashedString& sInteraction, float fInteractionImpulse, nsUInt32 uiIgnoreObjectID /*= nsInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<nsPhysicsWorldModuleInterface>())
  {
    nsPhysicsCastResult res;
    nsPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    if (pModule->Raycast(res, vRayStart, vRayDirection, 1.0f, params))
    {
      nsSurfaceResourceHandle hSurface = res.m_hSurface;
      if (!hSurface.IsValid() && !sFallbackSurface.IsEmpty())
      {
        hSurface = nsResourceManager::LoadResource<nsSurfaceResource>(sFallbackSurface);
      }

      if (hSurface.IsValid())
      {
        nsResourceLock<nsSurfaceResource> pSurf(hSurface, nsResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pSurf.GetAcquireResult() == nsResourceAcquireResult::Final)
        {
          return pSurf->InteractWithSurface(pWorld, {}, res.m_vPosition, res.m_vNormal, vRayDirection, sInteraction, nullptr, fInteractionImpulse);
        }
      }
    }
  }

  return false;
}


NS_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Physics);
