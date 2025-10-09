#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Recording/JvdRecordingTypes.h>

#include <Foundation/Containers/Map.h>

void nsJvdBodyMetadata::Reset()
{
  m_BodyGuid = {};
  m_uiBodyId = 0;
  m_uiSceneInstanceId = 0;
  m_sName.Clear();
  m_sLayer.Clear();
  m_sShape.Clear();
  m_sMaterial.Clear();
  m_bKinematic = false;
  m_bTrigger = false;
}

void nsJvdBodyState::Reset()
{
  m_uiBodyId = 0;
  m_vPosition.SetZero();
  m_qRotation.SetIdentity();
  m_vScale.Set(1.0f);
  m_vLinearVelocity.SetZero();
  m_vAngularVelocity.SetZero();
  m_fFriction = 0.0f;
  m_fRestitution = 0.0f;
  m_bIsSleeping = false;
  m_bWasTeleported = false;
  m_CustomProperties.Clear();
}

const nsJvdBodyState* nsJvdFrame::FindBody(nsUInt64 uiBodyId) const
{
  for (const auto& state : m_Bodies)
  {
    if (state.m_uiBodyId == uiBodyId)
      return &state;
  }

  return nullptr;
}

nsJvdBodyState* nsJvdFrame::FindBody(nsUInt64 uiBodyId)
{
  for (auto& state : m_Bodies)
  {
    if (state.m_uiBodyId == uiBodyId)
      return &state;
  }

  return nullptr;
}

void nsJvdFrame::AddOrUpdateBody(const nsJvdBodyState& state)
{
  if (nsJvdBodyState* pExisting = FindBody(state.m_uiBodyId))
  {
    *pExisting = state;
    return;
  }

  m_Bodies.PushBack(state);
}

void nsJvdClipMetadata::Reset()
{
  m_ClipGuid = nsUuid::MakeInvalid();
  m_sClipName.Clear();
  m_sAuthor.Clear();
  m_sSourceHost.Clear();
  m_Tags.Clear();
  m_CreationTimeUtc = nsTime::Now();
  m_SampleInterval = nsTime::MakeZero();
}

nsJvdClip::nsJvdClip()
{
  m_Metadata.Reset();
}

nsJvdClip::nsJvdClip(const nsJvdClip& other)
  : m_Metadata(other.m_Metadata)
  , m_Frames(other.m_Frames)
{
}

nsJvdClip::nsJvdClip(nsJvdClip&& other) noexcept
  : m_Metadata(std::move(other.m_Metadata))
  , m_Frames(std::move(other.m_Frames))
{
  other.m_Metadata.Reset();
  other.m_Frames.Clear();
}

nsJvdClip::~nsJvdClip() = default;

nsJvdClip& nsJvdClip::operator=(const nsJvdClip& other)
{
  if (this == &other)
    return *this;

  m_Metadata = other.m_Metadata;
  m_Frames = other.m_Frames;
  return *this;
}

nsJvdClip& nsJvdClip::operator=(nsJvdClip&& other) noexcept
{
  if (this == &other)
    return *this;

  m_Metadata = std::move(other.m_Metadata);
  m_Frames = std::move(other.m_Frames);
  other.m_Metadata.Reset();
  other.m_Frames.Clear();
  return *this;
}

void nsJvdClip::Clear()
{
  m_Metadata.Reset();
  m_Frames.Clear();
}

void nsJvdClip::SetMetadata(const nsJvdClipMetadata& metadata)
{
  m_Metadata = metadata;
  if (!m_Metadata.m_ClipGuid.IsValid())
  {
    m_Metadata.m_ClipGuid = nsUuid::MakeUuid();
  }
}

const nsJvdClipMetadata& nsJvdClip::GetMetadata() const
{
  return m_Metadata;
}

nsUInt64 nsJvdClip::AddFrame(nsJvdFrame&& frame)
{
  if (frame.m_uiFrameIndex == 0)
  {
    frame.m_uiFrameIndex = m_Frames.GetCount();
  }

  if (!m_Frames.IsEmpty())
  {
    // ensure strictly monotonic timestamps
    const nsTime lastTime = m_Frames.PeekBack().m_Timestamp;
    if (frame.m_Timestamp <= lastTime)
    {
      frame.m_Timestamp = lastTime + nsTime::MakeFromMicroseconds(1);
    }
  }

  m_Frames.PushBack(std::move(frame));
  return m_Frames.PeekBack().m_uiFrameIndex;
}

const nsJvdFrame* nsJvdClip::FindFrameByTime(nsTime timestamp) const
{
  if (m_Frames.IsEmpty())
    return nullptr;

  const nsUInt32 uiCount = m_Frames.GetCount();
  nsUInt32 uiLow = 0;
  nsUInt32 uiHigh = uiCount;

  while (uiLow < uiHigh)
  {
    const nsUInt32 uiMid = (uiLow + uiHigh) / 2;
    const nsJvdFrame& midFrame = m_Frames[uiMid];

    if (midFrame.m_Timestamp < timestamp)
    {
      uiLow = uiMid + 1;
    }
    else
    {
      uiHigh = uiMid;
    }
  }

  if (uiLow >= uiCount)
    return &m_Frames.PeekBack();

  return &m_Frames[uiLow];
}

const nsJvdFrame* nsJvdClip::FindFrame(nsUInt64 uiFrameIndex) const
{
  for (const auto& frame : m_Frames)
  {
    if (frame.m_uiFrameIndex == uiFrameIndex)
      return &frame;
  }
  return nullptr;
}

nsTime nsJvdClip::GetDuration() const
{
  if (m_Frames.IsEmpty())
    return nsTime::MakeZero();

  return m_Frames.PeekBack().m_Timestamp - m_Frames[0].m_Timestamp;
}

nsTime nsJvdClip::GetSampleInterval() const
{
  if (m_Metadata.m_SampleInterval.IsPositive())
    return m_Metadata.m_SampleInterval;

  if (m_Frames.GetCount() < 2)
    return nsTime::MakeZero();

  const nsTime total = m_Frames.PeekBack().m_Timestamp - m_Frames[0].m_Timestamp;
  return nsTime::MakeFromSeconds(total.GetSeconds() / (m_Frames.GetCount() - 1));
}

void nsJvdRecordingSettings::Reset()
{
  m_sSessionName.Clear();
  m_IncludedBodies.Clear();
  m_ExcludedBodies.Clear();
  m_TargetFrameInterval = nsTime::MakeFromSeconds(1.0 / 60.0);
  m_MaximumCaptureTime = nsTime::MakeZero();
  m_bCaptureSleepingBodies = false;
  m_bRecordVelocities = true;
  m_bRecordCustomProperties = false;
}

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdBodyMetadata, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdBodyMetadata>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_ACCESSOR_PROPERTY("BodyGuid", GetBodyGuid, SetBodyGuid),
    NS_MEMBER_PROPERTY("BodyId", m_uiBodyId),
    NS_MEMBER_PROPERTY("SceneInstanceId", m_uiSceneInstanceId),
    NS_MEMBER_PROPERTY("Name", m_sName),
    NS_MEMBER_PROPERTY("Layer", m_sLayer),
    NS_MEMBER_PROPERTY("Shape", m_sShape),
    NS_MEMBER_PROPERTY("Material", m_sMaterial),
    NS_MEMBER_PROPERTY("Kinematic", m_bKinematic),
    NS_MEMBER_PROPERTY("Trigger", m_bTrigger),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdBodyState, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdBodyState>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("BodyId", m_uiBodyId),
    NS_MEMBER_PROPERTY("Position", m_vPosition),
    NS_MEMBER_PROPERTY("Rotation", m_qRotation),
    NS_MEMBER_PROPERTY("Scale", m_vScale),
    NS_MEMBER_PROPERTY("LinearVelocity", m_vLinearVelocity),
    NS_MEMBER_PROPERTY("AngularVelocity", m_vAngularVelocity),
    NS_MEMBER_PROPERTY("Friction", m_fFriction),
    NS_MEMBER_PROPERTY("Restitution", m_fRestitution),
    NS_MEMBER_PROPERTY("Sleeping", m_bIsSleeping),
    NS_MEMBER_PROPERTY("Teleported", m_bWasTeleported),
    NS_MEMBER_PROPERTY("Custom", m_CustomProperties),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdFrame, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdFrame>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("FrameIndex", m_uiFrameIndex),
    NS_MEMBER_PROPERTY("Timestamp", m_Timestamp),
    NS_ARRAY_MEMBER_PROPERTY("Bodies", m_Bodies),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdClipMetadata, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdClipMetadata>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("ClipGuid", m_ClipGuid),
    NS_MEMBER_PROPERTY("ClipName", m_sClipName),
    NS_MEMBER_PROPERTY("Author", m_sAuthor),
    NS_MEMBER_PROPERTY("SourceHost", m_sSourceHost),
    NS_ARRAY_MEMBER_PROPERTY("Tags", m_Tags),
    NS_MEMBER_PROPERTY("CreationTimeUtc", m_CreationTimeUtc),
    NS_MEMBER_PROPERTY("SampleInterval", m_SampleInterval),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdClip, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdClip>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_ACCESSOR_PROPERTY("Metadata", GetMetadata, SetMetadata),
    NS_ARRAY_MEMBER_PROPERTY("Frames", m_Frames),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_TYPE(nsJvdRecordingSettings, nsNoBase, 1, nsRTTIDefaultAllocator<nsJvdRecordingSettings>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("SessionName", m_sSessionName),
    NS_ARRAY_MEMBER_PROPERTY("IncludedBodies", m_IncludedBodies),
    NS_ARRAY_MEMBER_PROPERTY("ExcludedBodies", m_ExcludedBodies),
    NS_MEMBER_PROPERTY("TargetFrameInterval", m_TargetFrameInterval),
    NS_MEMBER_PROPERTY("MaximumCaptureTime", m_MaximumCaptureTime),
    NS_MEMBER_PROPERTY("CaptureSleepingBodies", m_bCaptureSleepingBodies),
    NS_MEMBER_PROPERTY("RecordVelocities", m_bRecordVelocities),
    NS_MEMBER_PROPERTY("RecordCustomProperties", m_bRecordCustomProperties),
  }
  NS_END_PROPERTIES;
}
NS_END_STATIC_REFLECTED_TYPE;

NS_STATICLINK_FILE(JVDSDK, Recording_JvdRecordingTypes);
