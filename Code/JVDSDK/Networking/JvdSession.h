#pragma once

#include <JVDSDK/Networking/JvdTelemetryBridge.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>

struct NS_JVDSDK_DLL nsJvdSessionConfiguration
{
  nsString m_sEndpoint;
  nsString m_sSessionName;
  nsUInt16 m_uiPort = 1040;
  bool m_bStartAsServer = true;
};

class NS_JVDSDK_DLL nsJvdSession
{
public:
  nsJvdSession();
  ~nsJvdSession();

  nsResult Initialize(const nsJvdSessionConfiguration& config);
  void Shutdown();

  void Update();

  void BroadcastFrame(const nsJvdFrame& frame);
  void BroadcastClip(const nsJvdClip& clip);

  nsEvent<const nsJvdFrame&, nsMutex>& OnFrameReceived() { return m_FrameEvent; }
  nsEvent<const nsJvdClip&, nsMutex>& OnClipReceived() { return m_ClipEvent; }

  const nsJvdSessionConfiguration& GetConfiguration() const { return m_Config; }
  bool IsRunning() const { return m_bInitialized; }

private:
  void ForwardFrame(const nsJvdFrame& frame);
  void ForwardClip(const nsJvdClip& clip);

  nsJvdTelemetryBridge m_Telemetry;
  nsJvdSessionConfiguration m_Config;
  bool m_bInitialized = false;

  nsEvent<const nsJvdFrame&, nsMutex> m_FrameEvent;
  nsEvent<const nsJvdClip&, nsMutex> m_ClipEvent;

  nsEvent<const nsJvdFrame&, nsMutex>::Handler m_TelemetryFrameHandler;
  nsEvent<const nsJvdClip&, nsMutex>::Handler m_TelemetryClipHandler;
};
