#include <PvdRenderer/PvdRendererPCH.h>

#include <PvdRenderer/Renderer/PvdBodyPass.h>
#include <PvdRenderer/Renderer/PvdRenderData.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <Foundation/Types/TempHashedString.h>
#include <Foundation/Strings/HashedString.h>

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsPvdBodyPass, 1, nsRTTIDefaultAllocator<nsPvdBodyPass>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("Color", m_PinColor),
    NS_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;

nsPvdBodyPass::nsPvdBodyPass(const char* szName)
  : nsRenderPipelinePass(szName, true)
{
}

nsPvdBodyPass::~nsPvdBodyPass() = default;

bool nsPvdBodyPass::GetRenderTargetDescriptions(const nsView& view, const nsArrayPtr<nsGALTextureCreationDescription* const> inputs,
  nsArrayPtr<nsGALTextureCreationDescription> outputs)
{
  NS_IGNORE_UNUSED(view);

  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }

  return true;
}

void nsPvdBodyPass::Execute(const nsRenderViewContext& renderViewContext, const nsArrayPtr<nsRenderPipelinePassConnection* const> inputs,
  const nsArrayPtr<nsRenderPipelinePassConnection* const> outputs)
{
  NS_IGNORE_UNUSED(outputs);

  nsGALDevice* pDevice = nsGALDevice::GetDefaultDevice();

  nsGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex] != nullptr)
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex] != nullptr)
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = nsRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());
  NS_IGNORE_UNUSED(pCommandEncoder);

  nsTempHashedString sRenderPass = nsMakeHashedString("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != nsViewRenderMode::None)
  {
    sRenderPass = nsViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  RenderDataWithCategory(renderViewContext, nsPvdRenderDataCategories::Body);

  nsDebugRenderer::RenderWorldSpace(renderViewContext);
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Renderer_PvdBodyPass);
