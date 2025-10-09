#pragma once

#include <QBasicTimer>
#include <QEvent>
#include <QPaintEngine>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimerEvent>
#include <QWidget>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/UniquePtr.h>
#include <JVDSDK/Recording/JvdRecordingTypes.h>
#include <PvdRenderer/Renderer/PvdVulkanRenderer.h>

/// \brief Qt widget that embeds the renderer and visualizes JVD body snapshots.
class JDebugViewportWidget : public QWidget
{
  Q_OBJECT

public:
  explicit JDebugViewportWidget(QWidget* parent = nullptr);
  ~JDebugViewportWidget() override;

  /// \brief Replaces the current frame to visualize on the next render tick.
  void DisplayFrame(const nsJvdFrame& frame);
  void RetryRendererInitialization();

  bool IsRendererInitialized() const { return m_bRendererInitialized; }
  bool HasRendererFailed() const { return m_bRendererFailed; }

signals:
  void RendererStateChanged(bool bInitialized, bool bFailed);

protected:
  void showEvent(QShowEvent* event) override;
  void changeEvent(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  QPaintEngine* paintEngine() const override;
  void timerEvent(QTimerEvent* event) override;

private:
  struct QtWindowAdapter;

  void InitializeRenderer();
  void ShutdownRenderer();
  void UpdateCamera();
  void UpdateViewProjection();
  void RenderFrame();

  nsUniquePtr<QtWindowAdapter> m_pWindowAdapter;

  nsJvdFrame m_CurrentFrame;
  bool m_bHasFrame = false;
  nsCamera m_Camera;
  float m_fSceneRadius = 10.0f;
  nsUniquePtr<nsPvdVulkanRenderer> m_pVulkanRenderer;
  nsMat4 m_LastViewProjection = nsMat4::MakeIdentity();
  bool m_bViewProjectionValid = false;

  bool m_bRendererInitialized = false;
  bool m_bViewportDirty = false;
  bool m_bVisible = false;
  bool m_bRendererFailed = false;

  QBasicTimer m_FrameTimer;
};
