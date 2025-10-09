#pragma once

#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <PvdRenderer/Renderer/PvdRenderData.h>

class nsGALDevice;
class nsView;

/// \brief Vulkan-based renderer for PVD body debug visualization.
class nsPvdBodyRenderer : public nsRenderer
{
  NS_ADD_DYNAMIC_REFLECTION(nsPvdBodyRenderer, nsRenderer);

public:
  nsPvdBodyRenderer();
  ~nsPvdBodyRenderer();

protected:
  virtual void GetSupportedRenderDataTypes(nsHybridArray<const nsRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(nsHybridArray<nsRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(const nsRenderViewContext& renderViewContext, const nsRenderPipelinePass* pPass, const nsRenderDataBatch& batch) const override;

private:
  void EnsurePrimitiveMeshes() const;
  nsMeshBufferResourceHandle CreateUnitBoxMesh() const;
  nsMeshBufferResourceHandle CreateUnitSphereMesh(nsUInt32 uiSegments, nsUInt32 uiStacks) const;

  nsShaderResourceHandle m_hShader;
  nsConstantBufferStorageHandle m_hConstantBuffer;

  mutable nsMeshBufferResourceHandle m_hUnitBoxMesh;
  mutable nsMeshBufferResourceHandle m_hUnitSphereMesh;
};
