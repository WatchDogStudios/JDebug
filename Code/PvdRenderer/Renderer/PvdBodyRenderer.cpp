#include <PvdRenderer/PvdRendererPCH.h>
#include <PvdRenderer/Renderer/PvdBodyRenderer.h>

#include <PvdRenderer/Renderer/PvdRenderData.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <PvdRenderer/../../../Data/Base/Shaders/Pvd/PvdBodyConstants.h>

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsPvdBodyRenderer, 1, nsRTTIDefaultAllocator<nsPvdBodyRenderer>)
NS_END_DYNAMIC_REFLECTED_TYPE;

nsPvdBodyRenderer::nsPvdBodyRenderer()
{
  m_hShader = nsResourceManager::LoadResource<nsShaderResource>("Shaders/Pvd/PvdBody.nsShader");
  if (!m_hShader.IsValid())
  {
    nsLog::Error("Failed to load 'Shaders/Pvd/PvdBody.nsShader'. PVD bodies will not render.");
  }

  m_hConstantBuffer = nsRenderContext::CreateConstantBufferStorage<nsPvdBodyConstants>();
}

nsPvdBodyRenderer::~nsPvdBodyRenderer()
{
  if (m_hConstantBuffer.IsValid())
  {
    nsRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
    m_hConstantBuffer.Invalidate();
  }
}

void nsPvdBodyRenderer::GetSupportedRenderDataTypes(nsHybridArray<const nsRTTI*, 8>& types) const
{
  types.PushBack(nsGetStaticRTTI<nsPvdBodyRenderData>());
}

void nsPvdBodyRenderer::GetSupportedRenderDataCategories(nsHybridArray<nsRenderData::Category, 8>& categories) const
{
  categories.PushBack(nsPvdRenderDataCategories::Body);
}

void nsPvdBodyRenderer::RenderBatch(const nsRenderViewContext& renderViewContext, const nsRenderPipelinePass* pPass, const nsRenderDataBatch& batch) const
{
  NS_IGNORE_UNUSED(pPass);

  if (!m_hShader.IsValid())
    return;

  EnsurePrimitiveMeshes();

  nsRenderContext* pContext = renderViewContext.m_pRenderContext;
  pContext->BindShader(m_hShader);
  pContext->BindConstantBuffer("nsPvdBodyConstants", m_hConstantBuffer);

  for (auto it = batch.GetIterator<nsPvdBodyRenderData>(); it.IsValid(); ++it)
  {
    const nsPvdBodyRenderData* pData = it;

    nsMeshBufferResourceHandle hMeshToUse = m_hUnitBoxMesh;
    switch (pData->m_Shape.GetValue())
    {
      case nsJvdShapeType::Sphere:
        hMeshToUse = m_hUnitSphereMesh;
        break;

      default:
        hMeshToUse = m_hUnitBoxMesh;
        break;
    }

    if (!hMeshToUse.IsValid())
      continue;

    nsColor color = pData->m_Color;
    if (pData->m_bSleeping)
    {
      color *= 0.35f;
      color.a = pData->m_Color.a;
    }

    nsPvdBodyConstants* pConstants = nsRenderContext::GetConstantBufferData<nsPvdBodyConstants>(m_hConstantBuffer);
    pConstants->ObjectToWorldMatrix = pData->m_GlobalTransform.GetAsMat4();
    pConstants->Color = color;
    pConstants->GameObjectID = pData->m_uiUniqueID;
    pConstants->Padding = nsVec3::MakeZero();

    pContext->BindMeshBuffer(hMeshToUse);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

void nsPvdBodyRenderer::EnsurePrimitiveMeshes() const
{
  if (!m_hUnitBoxMesh.IsValid())
  {
    m_hUnitBoxMesh = CreateUnitBoxMesh();
  }

  if (!m_hUnitSphereMesh.IsValid())
  {
    m_hUnitSphereMesh = CreateUnitSphereMesh(24, 16);
  }
}

nsMeshBufferResourceHandle nsPvdBodyRenderer::CreateUnitBoxMesh() const
{
  const char* szResourceName = "PvdUnitBoxMeshBuffer";
  nsMeshBufferResourceHandle hMesh = nsResourceManager::GetExistingResource<nsMeshBufferResource>(szResourceName);
  if (hMesh.IsValid())
    return hMesh;

  nsMeshBufferResourceDescriptor desc;
  const nsUInt32 uiPositionStream = desc.AddStream(nsGALVertexAttributeSemantic::Position, nsGALResourceFormat::XYZFloat);

  desc.AllocateStreams(8, nsGALPrimitiveTopology::Triangles, 12);

  static const nsVec3 positions[8] = {
    nsVec3(-0.5f, -0.5f, -0.5f),
    nsVec3(0.5f, -0.5f, -0.5f),
    nsVec3(0.5f, 0.5f, -0.5f),
    nsVec3(-0.5f, 0.5f, -0.5f),
    nsVec3(-0.5f, -0.5f, 0.5f),
    nsVec3(0.5f, -0.5f, 0.5f),
    nsVec3(0.5f, 0.5f, 0.5f),
    nsVec3(-0.5f, 0.5f, 0.5f),
  };

  for (nsUInt32 i = 0; i < NS_ARRAY_SIZE(positions); ++i)
  {
    desc.SetVertexData(uiPositionStream, i, positions[i]);
  }

  static const nsUInt16 indices[36] = {
    4, 5, 6, 4, 6, 7, // front
    1, 0, 3, 1, 3, 2, // back
    0, 4, 7, 0, 7, 3, // left
    5, 1, 2, 5, 2, 6, // right
    3, 7, 6, 3, 6, 2, // top
    0, 1, 5, 0, 5, 4  // bottom
  };

  for (nsUInt32 tri = 0; tri < 12; ++tri)
  {
    const nsUInt32 idx = tri * 3;
    desc.SetTriangleIndices(tri, indices[idx + 0], indices[idx + 1], indices[idx + 2]);
  }

  hMesh = nsResourceManager::GetOrCreateResource<nsMeshBufferResource>(szResourceName, std::move(desc), szResourceName);
  return hMesh;
}

nsMeshBufferResourceHandle nsPvdBodyRenderer::CreateUnitSphereMesh(nsUInt32 uiSegments, nsUInt32 uiStacks) const
{
  const char* szResourceName = "PvdUnitSphereMeshBuffer";
  nsMeshBufferResourceHandle hMesh = nsResourceManager::GetExistingResource<nsMeshBufferResource>(szResourceName);
  if (hMesh.IsValid())
    return hMesh;

  uiSegments = nsMath::Max<nsUInt32>(uiSegments, 3);
  uiStacks = nsMath::Max<nsUInt32>(uiStacks, 2);

  nsMeshBufferResourceDescriptor desc;
  const nsUInt32 uiPositionStream = desc.AddStream(nsGALVertexAttributeSemantic::Position, nsGALResourceFormat::XYZFloat);

  const nsUInt32 uiVertexCount = (uiSegments + 1) * (uiStacks + 1);
  const nsUInt32 uiTriangleCount = uiSegments * uiStacks * 2;
  desc.AllocateStreams(uiVertexCount, nsGALPrimitiveTopology::Triangles, uiTriangleCount);

  nsUInt32 uiVertexIndex = 0;
  for (nsUInt32 stack = 0; stack <= uiStacks; ++stack)
  {
    const float v = static_cast<float>(stack) / static_cast<float>(uiStacks);
    const float phi = nsMath::Pi<float>() * v;
    const float sinPhi = nsMath::Sin(phi);
    const float cosPhi = nsMath::Cos(phi);

    for (nsUInt32 seg = 0; seg <= uiSegments; ++seg)
    {
      const float u = static_cast<float>(seg) / static_cast<float>(uiSegments);
      const float theta = 2.0f * nsMath::Pi<float>() * u;
      const float sinTheta = nsMath::Sin(theta);
      const float cosTheta = nsMath::Cos(theta);

      nsVec3 position(sinPhi * cosTheta * 0.5f, cosPhi * 0.5f, sinPhi * sinTheta * 0.5f);
      desc.SetVertexData(uiPositionStream, uiVertexIndex, position);
      ++uiVertexIndex;
    }
  }

  nsUInt32 uiTriangleIndex = 0;
  const nsUInt32 uiStride = uiSegments + 1;
  for (nsUInt32 stack = 0; stack < uiStacks; ++stack)
  {
    for (nsUInt32 seg = 0; seg < uiSegments; ++seg)
    {
      const nsUInt32 i0 = stack * uiStride + seg;
      const nsUInt32 i1 = (stack + 1) * uiStride + seg;
      const nsUInt32 i2 = stack * uiStride + (seg + 1);
      const nsUInt32 i3 = (stack + 1) * uiStride + (seg + 1);

      desc.SetTriangleIndices(uiTriangleIndex++, i0, i1, i2);
      desc.SetTriangleIndices(uiTriangleIndex++, i2, i1, i3);
    }
  }

  hMesh = nsResourceManager::GetOrCreateResource<nsMeshBufferResource>(szResourceName, std::move(desc), szResourceName);
  return hMesh;
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Renderer_PvdBodyRenderer);
