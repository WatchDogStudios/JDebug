#include <PvdRenderer/PvdRendererPCH.h>

#include <PvdRenderer/Renderer/PvdRenderData.h>

#include <Foundation/Math/HashingUtils.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

nsRenderData::Category nsPvdRenderDataCategories::Body =
  nsRenderData::RegisterCategory("PvdBody", &nsRenderSortingFunctions::ByRenderDataThenFrontToBack);

// Register the renderer for this category
#include <PvdRenderer/Renderer/PvdBodyRenderer.h>
NS_BEGIN_SUBSYSTEM_DECLARATION(PvdRenderer, PvdBodyRendererRegistration)

void OnEngineStartup()
{
  nsRenderData::AddRendererForCategory(nsPvdRenderDataCategories::Body, nsGetStaticRTTI<nsPvdBodyRenderer>());
}

void OnEngineShutdown() {}

NS_END_SUBSYSTEM_DECLARATION;

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsPvdBodyRenderData, 1, nsRTTIDefaultAllocator<nsPvdBodyRenderData>)
NS_END_DYNAMIC_REFLECTED_TYPE;

nsPvdBodyRenderData::nsPvdBodyRenderData()
{
  m_uiSubMeshIndex = 0;
  m_uiUniformScale = 1;
  m_uiFlipWinding = 0;
}

void nsPvdBodyRenderData::SetShape(nsEnum<nsJvdShapeType> shape, const nsVec3& dimensions)
{
  m_Shape = shape;
  m_vShapeDimensions = dimensions;
}

void nsPvdBodyRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(static_cast<nsUInt32>(m_Shape.GetValue()));

  const nsUInt32 uiBodyHash = nsHashingUtils::xxHash32(&m_uiBodyId, sizeof(m_uiBodyId));
  m_uiSortingKey = uiBodyHash;
}
