#include "JDebugViewportWidget.h"

#include <QCoreApplication>
#include <QEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QWindow>

#include <Core/System/Window.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/AtomicInteger.h>

#include <cmath>

namespace
{
  constexpr float s_fDefaultFov = 60.0f;
} // namespace

struct JDebugViewportWidget::QtWindowAdapter final : public nsWindowBase
{
  explicit QtWindowAdapter(QWidget& widget)
    : m_Widget(widget)
  {
  }

  nsSizeU32 GetClientAreaSize() const override
  {
    const QSize widgetSize = m_Widget.size();

    if (widgetSize.width() <= 0 || widgetSize.height() <= 0)
    {
      return {0u, 0u};
    }

    const qreal devicePixelRatio = m_Widget.devicePixelRatioF();
    const nsUInt32 uiWidth = nsMath::Max<nsUInt32>(1u, static_cast<nsUInt32>(std::lround(widgetSize.width() * devicePixelRatio)));
    const nsUInt32 uiHeight = nsMath::Max<nsUInt32>(1u, static_cast<nsUInt32>(std::lround(widgetSize.height() * devicePixelRatio)));
    return {uiWidth, uiHeight};
  }

  nsWindowHandle GetNativeWindowHandle() const override
  {
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
    return reinterpret_cast<nsWindowHandle>(m_Widget.winId());
#else
    nsLog::Error("QtWindowAdapter::GetNativeWindowHandle is not implemented for this platform.");
    return nsWindowHandle{};
#endif
  }

  bool IsFullscreenWindow(bool /*bOnlyProperFullscreenMode*/) const override { return false; }

  bool IsVisible() const override { return m_Widget.isVisible(); }

  void ProcessWindowMessages() override {}

  void AddReference() override { m_iRefCount.Increment(); }

  void RemoveReference() override { m_iRefCount.Decrement(); }

private:
  QWidget& m_Widget;
  mutable nsAtomicInteger32 m_iRefCount = 0;
};

JDebugViewportWidget::JDebugViewportWidget(QWidget* parent)
  : QWidget(parent)
{
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAutoFillBackground(false);

  // Ensure a native window is created so that winId() returns a valid handle.
  winId();

  if (!m_bRendererInitialized)
  {
    InitializeRenderer();
  }
}

JDebugViewportWidget::~JDebugViewportWidget()
{
  ShutdownRenderer();
}

void JDebugViewportWidget::DisplayFrame(const nsJvdFrame& frame)
{
  m_CurrentFrame = frame;
  m_bHasFrame = true;
  m_bViewportDirty = true;

  InitializeRenderer();

  if (isVisible())
  {
    update();
  }
}

void JDebugViewportWidget::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);

  m_bVisible = true;
  InitializeRenderer();

  if (!m_FrameTimer.isActive())
  {
    m_FrameTimer.start(0, this);
  }

  update();
}

void JDebugViewportWidget::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);

  if (event->type() == QEvent::Hide)
  {
    m_bVisible = false;

    if (m_FrameTimer.isActive())
    {
      m_FrameTimer.stop();
    }
  }
}

void JDebugViewportWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  m_bViewportDirty = true;

  if (m_pVulkanRenderer && m_pWindowAdapter)
  {
    const nsSizeU32 size = m_pWindowAdapter->GetClientAreaSize();
    m_pVulkanRenderer->SetBackBufferSize(size.width, size.height);
  }

  update();
}

void JDebugViewportWidget::paintEvent(QPaintEvent* event)
{
  NS_IGNORE_UNUSED(event);
  RenderFrame();
}

QPaintEngine* JDebugViewportWidget::paintEngine() const
{
  return nullptr;
}

void JDebugViewportWidget::timerEvent(QTimerEvent* event)
{
  if (event->timerId() == m_FrameTimer.timerId())
  {
    RenderFrame();
    return;
  }

  QWidget::timerEvent(event);
}

void JDebugViewportWidget::InitializeRenderer()
{
  if (m_bRendererInitialized || m_bRendererFailed)
    return;

  if (!isVisible())
    return;

  if (QWindow* pWindow = windowHandle())
  {
    if (!pWindow->isExposed())
      return;
  }

  if (!m_pWindowAdapter)
  {
    m_pWindowAdapter = NS_DEFAULT_NEW(QtWindowAdapter, *this);
  }

  const nsWindowHandle nativeHandle = m_pWindowAdapter->GetNativeWindowHandle();
  if (nativeHandle == INVALID_WINDOW_HANDLE_VALUE)
    return;

  nsSizeU32 initialSize = m_pWindowAdapter->GetClientAreaSize();
  if (initialSize.width == 0 || initialSize.height == 0)
    return;

  if (!m_pVulkanRenderer)
  {
    nsVulkanRendererCreateInfo rendererInfo;
    rendererInfo.m_pWindowHandle = reinterpret_cast<void*>(nativeHandle);
    rendererInfo.m_uiWidth = initialSize.width;
    rendererInfo.m_uiHeight = initialSize.height;
    rendererInfo.m_bEnableValidation = true;

    m_pVulkanRenderer = NS_DEFAULT_NEW(nsPvdVulkanRenderer);
    if (m_pVulkanRenderer->Initialize(rendererInfo).Failed())
    {
      nsLog::Error("Failed to initialize Vulkan renderer for the PVD viewport.");
      m_pVulkanRenderer.Clear();
      m_bRendererFailed = true;
      emit RendererStateChanged(false, true);
      return;
    }
  }

  m_pVulkanRenderer->SetBackBufferSize(initialSize.width, initialSize.height);
  m_bRendererFailed = false;
  m_bViewportDirty = true;
  m_bRendererInitialized = true;
  emit RendererStateChanged(true, false);
}

void JDebugViewportWidget::ShutdownRenderer()
{
  if (m_FrameTimer.isActive())
  {
    m_FrameTimer.stop();
  }

  if (m_pVulkanRenderer)
  {
    m_pVulkanRenderer->Deinitialize();
    m_pVulkanRenderer.Clear();
  }
  m_pWindowAdapter.Clear();

  m_bHasFrame = false;
  m_bViewportDirty = false;
  m_bViewProjectionValid = false;

  m_bRendererInitialized = false;
  m_bRendererFailed = false;

  emit RendererStateChanged(false, false);
}

void JDebugViewportWidget::RetryRendererInitialization()
{
  const bool bWasVisible = isVisible();

  ShutdownRenderer();

  if (bWasVisible && !m_FrameTimer.isActive())
  {
    m_FrameTimer.start(0, this);
  }

  InitializeRenderer();

  if (m_bRendererInitialized)
  {
    update();
  }
}

void JDebugViewportWidget::UpdateCamera()
{
  if (!m_bHasFrame)
    return;

  nsBoundingBox bounds = nsBoundingBox::MakeInvalid();

  for (const nsJvdBodyState& state : m_CurrentFrame.m_Bodies)
  {
    const nsVec3 vHalfExtents = state.m_vScale * 0.5f;
    bounds.ExpandToInclude(state.m_vPosition - vHalfExtents);
    bounds.ExpandToInclude(state.m_vPosition + vHalfExtents);
  }

  if (!bounds.IsValid())
  {
    const nsVec3 vEye(-10.0f, 0.0f, 6.0f);
    const nsVec3 vTarget = nsVec3::MakeZero();
    m_Camera.LookAt(vEye, vTarget, nsVec3(0, 0, 1));
    m_fSceneRadius = 10.0f;
    return;
  }

  const nsVec3 vCenter = bounds.GetCenter();
  const nsVec3 vExtents = bounds.GetHalfExtents();
  m_fSceneRadius = nsMath::Max(vExtents.GetLength(), 1.0f);

  const nsVec3 vEyeOffset(-m_fSceneRadius * 2.5f, -m_fSceneRadius * 0.5f, m_fSceneRadius * 1.5f);
  m_Camera.LookAt(vCenter + vEyeOffset, vCenter, nsVec3(0, 0, 1));
}

void JDebugViewportWidget::UpdateViewProjection()
{
  nsSizeU32 viewportSize = m_pWindowAdapter ? m_pWindowAdapter->GetClientAreaSize() : nsSizeU32{static_cast<nsUInt32>(width()), static_cast<nsUInt32>(height())};
  if (viewportSize.width == 0 || viewportSize.height == 0)
  {
    m_bViewProjectionValid = false;
    return;
  }

  const float fAspect = static_cast<float>(viewportSize.width) / static_cast<float>(viewportSize.height);
  const float fNearPlane = nsMath::Max(0.1f, m_fSceneRadius * 0.05f);
  const float fFarPlane = nsMath::Max(100.0f, m_fSceneRadius * 6.0f);

  m_Camera.SetCameraMode(nsCameraMode::PerspectiveFixedFovY, s_fDefaultFov, fNearPlane, fFarPlane);

  nsMat4 projection = nsMat4::MakeIdentity();
  m_Camera.GetProjectionMatrix(fAspect, projection, nsCameraEye::Left, nsClipSpaceDepthRange::ZeroToOne);
  const nsMat4& viewMatrix = m_Camera.GetViewMatrix();
  m_LastViewProjection = viewMatrix * projection;
  m_bViewProjectionValid = true;
}

void JDebugViewportWidget::RenderFrame()
{
  if (!m_bRendererInitialized && !m_bRendererFailed)
  {
    InitializeRenderer();
  }

  if (!m_bRendererInitialized)
    return;

  if (!m_bVisible)
    return;

  if (!m_bViewportDirty && !m_bHasFrame)
    return;

  if (!m_pWindowAdapter)
    return;

  const nsSizeU32 viewportSize = m_pWindowAdapter->GetClientAreaSize();

  if (m_pVulkanRenderer)
  {
    m_pVulkanRenderer->SetBackBufferSize(viewportSize.width, viewportSize.height);
  }

  if (viewportSize.width == 0 || viewportSize.height == 0)
  {
    m_bViewportDirty = false;
    return;
  }

  if (m_bHasFrame)
  {
    UpdateCamera();
    if (m_pVulkanRenderer)
    {
      m_pVulkanRenderer->UpdateFrame(m_CurrentFrame);
    }
    m_bHasFrame = false;
  }

  UpdateViewProjection();

  if (m_pVulkanRenderer && m_bViewProjectionValid)
  {
    if (m_pVulkanRenderer->Render(m_LastViewProjection).Failed())
    {
      nsLog::Error("Vulkan renderer failed to render viewport frame.");
    }
  }

  m_bViewportDirty = false;
}
