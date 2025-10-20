#include <PvdRenderer/PvdRendererPCH.h>

#include <PvdRenderer/Renderer/PvdDirectX11Renderer.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Transform.h>

nsPvdDirectX11Renderer::nsPvdDirectX11Renderer()
  : m_ColorActive(0.95f, 0.55f, 0.25f)
  , m_ColorSleeping(0.35f, 0.5f, 0.9f)
{
}

nsPvdDirectX11Renderer::~nsPvdDirectX11Renderer()
{
  Deinitialize();
}

nsResult nsPvdDirectX11Renderer::Initialize(const nsDirectX11RendererCreateInfo& createInfo)
{
  Deinitialize();

  nsLog::Info("nsPvdDirectX11Renderer: Initializing DirectX11 renderer (windowHandle={0}, size={1}x{2}, debug={3})",
    nsArgP(createInfo.m_pWindowHandle),
    createInfo.m_uiWidth,
    createInfo.m_uiHeight,
    createInfo.m_bEnableDebugLayer ? "true" : "false");

  m_pRenderer = NS_DEFAULT_NEW(nsDirectX11Renderer);
  if (m_pRenderer->Initialize(createInfo).Failed())
  {
    nsLog::Error("nsPvdDirectX11Renderer: Failed to initialize DirectX11 renderer instance.");
    m_pRenderer.Clear();
    return NS_FAILURE;
  }

  if (createInfo.m_uiWidth > 0 && createInfo.m_uiHeight > 0)
  {
    m_pRenderer->SetBackBufferSize(createInfo.m_uiWidth, createInfo.m_uiHeight);
  }

  nsLog::Success("nsPvdDirectX11Renderer: DirectX11 renderer initialized successfully.");

  return NS_SUCCESS;
}

void nsPvdDirectX11Renderer::Deinitialize()
{
  if (m_pRenderer)
  {
    m_pRenderer->Deinitialize();
    m_pRenderer.Clear();
  }

  m_Instances.Clear();
}

bool nsPvdDirectX11Renderer::IsInitialized() const
{
  return m_pRenderer != nullptr && m_pRenderer->IsInitialized();
}

void nsPvdDirectX11Renderer::SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight)
{
  if (m_pRenderer)
  {
    m_pRenderer->SetBackBufferSize(uiWidth, uiHeight);
  }
}

void nsPvdDirectX11Renderer::UpdateFrame(const nsJvdFrame& frame)
{
  ConvertFrameToInstances(frame);
}

nsResult nsPvdDirectX11Renderer::Render(const nsMat4& mViewProjection)
{
  if (!m_pRenderer)
  {
    return NS_FAILURE;
  }

  nsArrayPtr<const nsDirectX11InstanceData> instances(m_Instances.GetData(), m_Instances.GetCount());
  m_pRenderer->UpdateScene(mViewProjection, instances);
  return m_pRenderer->RenderFrame();
}

void nsPvdDirectX11Renderer::SetBodyColorPalette(const nsColor& activeColor, const nsColor& sleepingColor)
{
  m_ColorActive = activeColor;
  m_ColorSleeping = sleepingColor;
}

void nsPvdDirectX11Renderer::ConvertFrameToInstances(const nsJvdFrame& frame)
{
  m_Instances.SetCount(frame.m_Bodies.GetCount());

  for (nsUInt32 i = 0; i < frame.m_Bodies.GetCount(); ++i)
  {
    const nsJvdBodyState& body = frame.m_Bodies[i];
    nsDirectX11InstanceData& instance = m_Instances[i];

    const nsVec3 vDimensions = body.m_vScale.CompMax(nsVec3(0.1f));

    nsTransform transform;
    transform.SetIdentity();
    transform.m_vPosition = body.m_vPosition;
    transform.m_qRotation = body.m_qRotation;
    transform.m_vScale = vDimensions;

    instance.m_ModelMatrix = transform.GetAsMat4();
    instance.m_Color = body.m_bIsSleeping ? m_ColorSleeping : m_ColorActive;
    instance.m_bSleeping = body.m_bIsSleeping;
  }
}

NS_STATICLINK_FILE(PvdRenderer, PvdRenderer_Renderer_PvdDirectX11Renderer);
