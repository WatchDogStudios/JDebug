#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Networking/JvdTelemetryBridge.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

nsJvdTelemetryBridge::nsJvdTelemetryBridge() = default;

nsJvdTelemetryBridge::~nsJvdTelemetryBridge()
{
  Shutdown();
}

nsResult nsJvdTelemetryBridge::StartServer(nsUInt16 uiPort, nsStringView sServerName)
{
  Shutdown();

  nsTelemetry::s_uiPort = uiPort;
  nsTelemetry::CreateServer();
  nsTelemetry::SetServerName(sServerName);

  m_uiPort = uiPort;
  m_bConnected = false;
  RegisterCallbacks();

  nsLog::Info("nsJvdTelemetryBridge started as server on port {0}", uiPort);
  return NS_SUCCESS;
}

nsResult nsJvdTelemetryBridge::ConnectToServer(nsStringView sAddress, nsUInt16 uiPort)
{
  Shutdown();

  nsTelemetry::s_uiPort = uiPort;
  nsResult result = nsTelemetry::ConnectToServer(sAddress);
  if (result.Failed())
  {
    nsLog::Warning("nsJvdTelemetryBridge failed to connect to {0}:{1}", sAddress, uiPort);
    return result;
  }

  m_uiPort = uiPort;
  RegisterCallbacks();

  nsLog::Info("nsJvdTelemetryBridge connected to {0}:{1}", sAddress, uiPort);
  return NS_SUCCESS;
}

void nsJvdTelemetryBridge::Shutdown()
{
  if (m_bCallbackRegistered)
  {
    UnregisterCallbacks();
  }

  if (nsTelemetry::GetConnectionMode() != nsTelemetry::ConnectionMode::None)
  {
    nsTelemetry::CloseConnection();
  }

  m_bConnected = false;
  m_uiPort = 0;
}

void nsJvdTelemetryBridge::Update()
{
  if (nsTelemetry::GetConnectionMode() == nsTelemetry::ConnectionMode::None)
    return;

  nsTelemetry::UpdateNetwork();
  nsTelemetry::PerFrameUpdate();
  ProcessIncomingMessages();
}

void nsJvdTelemetryBridge::SendFrame(const nsJvdFrame& frame)
{
  if (nsTelemetry::GetConnectionMode() == nsTelemetry::ConnectionMode::None)
    return;

  nsTelemetryMessage message;
  message.SetMessageID(nsJvdIds::g_uiTelemetrySystemId, nsJvdIds::g_uiTelemetryFrameMessageId);

  if (nsJvdSerialization::WriteFrame(message.GetWriter(), frame).Failed())
  {
    nsLog::Error("Failed to serialize frame for telemetry broadcast.");
    return;
  }

  nsTelemetry::Broadcast(nsTelemetry::TransmitMode::Unreliable, message);
}

void nsJvdTelemetryBridge::SendClip(const nsJvdClip& clip)
{
  if (nsTelemetry::GetConnectionMode() == nsTelemetry::ConnectionMode::None)
    return;

  nsTelemetryMessage message;
  message.SetMessageID(nsJvdIds::g_uiTelemetrySystemId, nsJvdIds::g_uiTelemetryClipMessageId);

  if (nsJvdSerialization::WriteClip(message.GetWriter(), clip).Failed())
  {
    nsLog::Error("Failed to serialize clip for telemetry broadcast.");
    return;
  }

  nsTelemetry::Broadcast(nsTelemetry::TransmitMode::Reliable, message);
}

void nsJvdTelemetryBridge::TelemetryMessageCallback(void* pPassThrough)
{
  auto* pSelf = static_cast<nsJvdTelemetryBridge*>(pPassThrough);
  if (pSelf)
  {
    pSelf->ProcessIncomingMessages();
  }
}

void nsJvdTelemetryBridge::OnTelemetryEvent(const nsTelemetry::TelemetryEventData& data)
{
  switch (data.m_EventType)
  {
    case nsTelemetry::TelemetryEventData::ConnectedToClient:
    case nsTelemetry::TelemetryEventData::ConnectedToServer:
      m_bConnected = true;
      break;

    case nsTelemetry::TelemetryEventData::DisconnectedFromClient:
    case nsTelemetry::TelemetryEventData::DisconnectedFromServer:
      m_bConnected = false;
      break;

    default:
      break;
  }
}

void nsJvdTelemetryBridge::ProcessIncomingMessages()
{
  nsTelemetryMessage message;
  while (nsTelemetry::RetrieveMessage(nsJvdIds::g_uiTelemetrySystemId, message).Succeeded())
  {
    auto& reader = static_cast<nsMemoryStreamReader&>(message.GetReader());
    reader.SetReadPosition(0);

    switch (message.GetMessageID())
    {
      case nsJvdIds::g_uiTelemetryFrameMessageId:
      {
        nsJvdFrame frame;
        if (nsJvdSerialization::ReadFrame(reader, frame).Succeeded())
        {
          m_FrameEvent.Broadcast(frame);
        }
        else
        {
          nsLog::Warning("nsJvdTelemetryBridge: Failed to deserialize telemetry frame message.");
        }
        break;
      }

      case nsJvdIds::g_uiTelemetryClipMessageId:
      {
        nsJvdClip clip;
        if (nsJvdSerialization::ReadClip(reader, clip).Succeeded())
        {
          m_ClipEvent.Broadcast(clip);
        }
        else
        {
          nsLog::Warning("nsJvdTelemetryBridge: Failed to deserialize telemetry clip message.");
        }
        break;
      }

      default:
        break;
    }
  }
}

void nsJvdTelemetryBridge::RegisterCallbacks()
{
  if (m_bCallbackRegistered)
    return;

  nsTelemetry::AcceptMessagesForSystem(nsJvdIds::g_uiTelemetrySystemId, true, &TelemetryMessageCallback, this);

  if (!m_TelemetryEventHandler.IsValid())
  {
    m_TelemetryEventHandler = nsMakeDelegate(&nsJvdTelemetryBridge::OnTelemetryEvent, this);
    nsTelemetry::AddEventHandler(m_TelemetryEventHandler);
  }

  m_bCallbackRegistered = true;
}

void nsJvdTelemetryBridge::UnregisterCallbacks()
{
  if (m_bCallbackRegistered)
  {
    nsTelemetry::AcceptMessagesForSystem(nsJvdIds::g_uiTelemetrySystemId, false, nullptr, nullptr);
    m_bCallbackRegistered = false;
  }

  if (m_TelemetryEventHandler.IsValid())
  {
    nsTelemetry::RemoveEventHandler(m_TelemetryEventHandler);
    m_TelemetryEventHandler = {};
  }
}

NS_STATICLINK_FILE(JVDSDK, Networking_JvdTelemetryBridge);
