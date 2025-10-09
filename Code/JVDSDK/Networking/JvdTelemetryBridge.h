#pragma once

#include <JVDSDK/Recording/JvdRecordingTypes.h>
#include <JVDSDK/Serialization/JvdStreamSerializer.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Strings/String.h>

class NS_JVDSDK_DLL nsJvdTelemetryBridge
{
public:
  nsJvdTelemetryBridge();
  ~nsJvdTelemetryBridge();

  nsResult StartServer(nsUInt16 uiPort, nsStringView sServerName);
  nsResult ConnectToServer(nsStringView sAddress, nsUInt16 uiPort);
  void Shutdown();

  bool IsConnected() const { return m_bConnected; }

  void Update();

  void SendFrame(const nsJvdFrame& frame);
  void SendClip(const nsJvdClip& clip);

  nsEvent<const nsJvdFrame&, nsMutex>& OnFrameReceived() { return m_FrameEvent; }
  nsEvent<const nsJvdClip&, nsMutex>& OnClipReceived() { return m_ClipEvent; }

private:
  static void TelemetryMessageCallback(void* pPassThrough);
  void OnTelemetryEvent(const nsTelemetry::TelemetryEventData& data);
  void ProcessIncomingMessages();

  void RegisterCallbacks();
  void UnregisterCallbacks();

  nsEvent<const nsJvdFrame&, nsMutex> m_FrameEvent;
  nsEvent<const nsJvdClip&, nsMutex> m_ClipEvent;

  bool m_bCallbackRegistered = false;
  bool m_bConnected = false;
  nsUInt16 m_uiPort = 0;
  nsTelemetry::nsEventTelemetry::Handler m_TelemetryEventHandler;
};
