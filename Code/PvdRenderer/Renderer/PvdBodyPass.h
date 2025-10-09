#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/Pipeline/RenderPipeline.h>

class NS_PVDRENDERER_DLL nsPvdBodyPass : public nsRenderPipelinePass
{
  NS_ADD_DYNAMIC_REFLECTION(nsPvdBodyPass, nsRenderPipelinePass);

public:
  nsPvdBodyPass(const char* szName = "PvdBodyPass");
  ~nsPvdBodyPass();

  virtual bool GetRenderTargetDescriptions(const nsView& view, const nsArrayPtr<nsGALTextureCreationDescription* const> inputs,
    nsArrayPtr<nsGALTextureCreationDescription> outputs) override;

  virtual void Execute(const nsRenderViewContext& renderViewContext, const nsArrayPtr<nsRenderPipelinePassConnection* const> inputs,
    const nsArrayPtr<nsRenderPipelinePassConnection* const> outputs) override;

protected:
  nsRenderPipelineNodePin m_PinColor;
  nsRenderPipelineNodePin m_PinDepthStencil;
};
