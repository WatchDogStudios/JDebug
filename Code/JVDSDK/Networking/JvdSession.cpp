#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Networking/JvdSession.h>

#include <Foundation/Logging/Log.h>

nsJvdSession::nsJvdSession() = default;

nsJvdSession::~nsJvdSession()
{
  Shutdown();
}

nsResult nsJvdSession::Initialize(const nsJvdSessionConfiguration& config)
{
  if (m_bInitialized)
  {
    Shutdown();
  }

  m_Config = config;

  nsResult result = NS_FAILURE;
  if (config.m_bStartAsServer)
  {
    result = m_Telemetry.StartServer(config.m_uiPort, config.m_sSessionName);
  }
  else
  {
    result = m_Telemetry.ConnectToServer(config.m_sEndpoint, config.m_uiPort);
  }

  if (result.Failed())
  {
    nsLog::Error("nsJvdSession failed to initialize telemetry bridge.");
    return result;
  }

  m_TelemetryFrameHandler = nsMakeDelegate(&nsJvdSession::ForwardFrame, this);
  m_TelemetryClipHandler = nsMakeDelegate(&nsJvdSession::ForwardClip, this);

  m_Telemetry.OnFrameReceived().AddEventHandler(m_TelemetryFrameHandler);
  m_Telemetry.OnClipReceived().AddEventHandler(m_TelemetryClipHandler);

  m_bInitialized = true;
  return NS_SUCCESS;
}

void nsJvdSession::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_Telemetry.OnFrameReceived().RemoveEventHandler(m_TelemetryFrameHandler);
  m_Telemetry.OnClipReceived().RemoveEventHandler(m_TelemetryClipHandler);

  m_Telemetry.Shutdown();

  m_FrameEvent.Clear();
  m_ClipEvent.Clear();

  m_bInitialized = false;
}

void nsJvdSession::Update()
{
  if (!m_bInitialized)
    return;

  m_Telemetry.Update();
}

void nsJvdSession::BroadcastFrame(const nsJvdFrame& frame)
{
  if (!m_bInitialized)
    return;

  m_Telemetry.SendFrame(frame);
}

void nsJvdSession::BroadcastClip(const nsJvdClip& clip)
{
  if (!m_bInitialized)
    return;

  m_Telemetry.SendClip(clip);
}

void nsJvdSession::ForwardFrame(const nsJvdFrame& frame)
{
  m_FrameEvent.Broadcast(frame);
}

void nsJvdSession::ForwardClip(const nsJvdClip& clip)
{
  m_ClipEvent.Broadcast(clip);
}

NS_STATICLINK_FILE(JVDSDK, Networking_JvdSession);
