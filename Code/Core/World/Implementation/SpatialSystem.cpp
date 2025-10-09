#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsSpatialSystem, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

nsSpatialSystem::nsSpatialSystem()
  : m_Allocator("Spatial System", nsFoundation::GetDefaultAllocator())
{
}

nsSpatialSystem::~nsSpatialSystem() = default;

void nsSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void nsSpatialSystem::FindObjectsInSphere(const nsBoundingSphere& sphere, const QueryParams& queryParams, nsDynamicArray<nsGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](nsGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return nsVisitorExecution::Continue;
    });
}

void nsSpatialSystem::FindObjectsInBox(const nsBoundingBox& box, const QueryParams& queryParams, nsDynamicArray<nsGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](nsGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return nsVisitorExecution::Continue;
    });
}

#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
void nsSpatialSystem::GetInternalStats(nsStringBuilder& ref_sSb) const
{
  ref_sSb.Clear();
}
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_Spatial, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(FindClosestObjectInSphere, In, "World", In, "Category", In, "Center", In, "Radius"),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("Spatial"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on


nsGameObject* nsScriptExtensionClass_Spatial::FindClosestObjectInSphere(nsWorld* pWorld, nsStringView sCategory, const nsVec3& vCenter, float fRadius)
{
  nsGameObject* pClosest = nullptr;

  auto category = nsSpatialData::FindCategory(sCategory);
  if (category != nsInvalidSpatialDataCategory)
  {
    nsSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    float fDistanceSqr = nsMath::HighValue<float>();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(nsBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [&](nsGameObject* go) -> nsVisitorExecution::Enum
      {
        const float fSqr = go->GetGlobalPosition().GetSquaredDistanceTo(vCenter);

        if (fSqr < fDistanceSqr)
        {
          fDistanceSqr = fSqr;
          pClosest = go;
        }

        return nsVisitorExecution::Continue;
        //
      });
  }

  return pClosest;
}

NS_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
