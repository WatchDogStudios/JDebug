#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Recording/JvdRecorder.h>

#include <JVDSDK/Serialization/JvdFileIO.h>

#include <Foundation/Strings/StringBuilder.h>

#include <Jolt/Physics/PhysicsSystem.h>

using namespace nsJvdConversion;

namespace
{
  nsTime MakeRelative(nsTime baseTime, nsTime timestamp)
  {
    if (baseTime.IsZero())
      return timestamp;

    nsTime relative = timestamp - baseTime;
    if (relative.IsNegative())
      return nsTime::MakeZero();

    return relative;
  }

  nsUInt64 MakeBodyKey(const JPH::BodyID& bodyId)
  {
    return static_cast<nsUInt64>(bodyId.GetIndexAndSequenceNumber());
  }
}

nsJvdRecorder::nsJvdRecorder()
{
  m_Metadata.Reset();
}

nsJvdRecorder::~nsJvdRecorder() = default;

void nsJvdRecorder::StartRecording(const nsJvdRecordingSettings& settings, const nsJvdClipMetadata& metadata)
{
  NS_LOCK(m_Mutex);

  m_Settings = settings;
  m_Settings.m_TargetFrameInterval = settings.m_TargetFrameInterval;

  m_Metadata = metadata;
  if (!m_Settings.m_sSessionName.IsEmpty() && m_Metadata.m_sClipName.IsEmpty())
  {
    m_Metadata.m_sClipName = m_Settings.m_sSessionName;
  }

  EnsureClipMetadata();
  m_Metadata.m_SampleInterval = m_Settings.m_TargetFrameInterval;

  m_Clip.Clear();
  m_Clip.SetMetadata(m_Metadata);
  m_BodyMetadata.Clear();

  m_bRecording = true;
  m_StartTime = nsTime::MakeZero();
  m_LastSampleTime = nsTime::MakeZero();
}

nsResult nsJvdRecorder::StopRecording(nsJvdClip& out_clip)
{
  NS_LOCK(m_Mutex);

  if (!m_bRecording)
    return NS_FAILURE;

  m_bRecording = false;
  out_clip = std::move(m_Clip);
  m_Clip.Clear();
  return NS_SUCCESS;
}

void nsJvdRecorder::CancelRecording()
{
  NS_LOCK(m_Mutex);
  m_bRecording = false;
  m_Clip.Clear();
  m_BodyMetadata.Clear();
}

void nsJvdRecorder::SetMetadata(const nsJvdClipMetadata& metadata)
{
  NS_LOCK(m_Mutex);
  m_Metadata = metadata;
  EnsureClipMetadata();
  m_Clip.SetMetadata(m_Metadata);
}

void nsJvdRecorder::AppendFrame(nsTime timestamp, nsArrayPtr<const nsJvdBodyState> states)
{
  NS_LOCK(m_Mutex);

  if (!m_bRecording)
    return;

  if (m_StartTime.IsZero())
  {
    m_StartTime = timestamp;
    m_LastSampleTime = nsTime::MakeZero();
  }

  const nsTime relative = MakeRelative(m_StartTime, timestamp);

  if (m_Settings.m_MaximumCaptureTime.IsPositive() && relative > m_Settings.m_MaximumCaptureTime)
  {
    nsLog::Warning("nsJvdRecorder::AppendFrame() - Maximum capture time reached. Frame discarded.");
    return;
  }

  if (!m_Clip.IsEmpty() && m_Settings.m_TargetFrameInterval.IsPositive())
  {
    const nsTime delta = relative - m_LastSampleTime;
    if (delta < m_Settings.m_TargetFrameInterval * 0.5)
    {
      return;
    }
  }

  nsJvdFrame frame;
  frame.m_uiFrameIndex = m_Clip.GetFrames().GetCount();
  frame.m_Timestamp = relative;
  frame.m_Bodies.PushBackRange(states);

  m_Clip.AddFrame(std::move(frame));
  m_LastSampleTime = relative;
}

nsResult nsJvdRecorder::CapturePhysicsSystem(const JPH::PhysicsSystem& physicsSystem, nsTime timestamp)
{
  const JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
  JPH::BodyIDVector bodyIds;
  physicsSystem.GetBodies(bodyIds);

  return CaptureBodies(bodyInterface, nsArrayPtr<const JPH::BodyID>(bodyIds.data(), static_cast<nsUInt32>(bodyIds.size())), timestamp);
}

nsResult nsJvdRecorder::CaptureBodies(const JPH::BodyInterface& bodyInterface, nsArrayPtr<const JPH::BodyID> bodyIds, nsTime timestamp)
{
  nsHybridArray<nsJvdBodyState, 64> states;
  states.Reserve(bodyIds.GetCount());

  for (const JPH::BodyID& bodyId : bodyIds)
  {
    if (bodyId.IsInvalid())
      continue;

    const nsUInt64 uiBodyKey = MakeBodyKey(bodyId);

    const bool bIsActive = bodyInterface.IsActive(bodyId);
    if (!ShouldCaptureBody(uiBodyKey, !bIsActive))
      continue;

    UpdateBodyMetadata(bodyInterface, bodyId);

    nsJvdBodyState state;
    state.m_uiBodyId = uiBodyKey;
    state.m_vPosition = ToVec3(bodyInterface.GetPosition(bodyId));
    state.m_qRotation = ToQuat(bodyInterface.GetRotation(bodyId));
    state.m_vScale.Set(1.0f);

    if (m_Settings.m_bRecordVelocities)
    {
      state.m_vLinearVelocity = ToVec3(bodyInterface.GetLinearVelocity(bodyId));
      state.m_vAngularVelocity = ToVec3(bodyInterface.GetAngularVelocity(bodyId));
    }

    state.m_fFriction = bodyInterface.GetFriction(bodyId);
    state.m_fRestitution = bodyInterface.GetRestitution(bodyId);
    state.m_bIsSleeping = !bIsActive;
    state.m_bWasTeleported = false;

    states.PushBack(std::move(state));
  }

  if (states.IsEmpty())
    return NS_SUCCESS;

  AppendFrame(timestamp, states.GetArrayPtr());
  return NS_SUCCESS;
}

bool nsJvdRecorder::ShouldCaptureBody(nsUInt64 uiBodyId, bool bIsSleeping) const
{
  if (!m_Settings.m_IncludedBodies.IsEmpty())
  {
    bool bFound = false;
    for (nsUInt64 includeId : m_Settings.m_IncludedBodies)
    {
      if (includeId == uiBodyId)
      {
        bFound = true;
        break;
      }
    }

    if (!bFound)
      return false;
  }

  for (nsUInt64 excludeId : m_Settings.m_ExcludedBodies)
  {
    if (excludeId == uiBodyId)
      return false;
  }

  if (!m_Settings.m_bCaptureSleepingBodies && bIsSleeping)
    return false;

  return true;
}

void nsJvdRecorder::UpdateBodyMetadata(const JPH::BodyInterface& bodyInterface, const JPH::BodyID& bodyId)
{
  const nsUInt64 uiBodyKey = MakeBodyKey(bodyId);

  bool bExisted = false;
  auto it = m_BodyMetadata.FindOrAdd(uiBodyKey, &bExisted);
  nsJvdBodyMetadata& metadata = it.Value();

  if (!bExisted)
  {
    metadata.Reset();
    metadata.m_uiBodyId = uiBodyKey;
  metadata.m_BodyGuid = nsUuid::MakeUuid();

  nsStringBuilder tmp;
  tmp.SetFormat("Body_{0}", uiBodyKey);
    metadata.m_sName = tmp;
  }

  metadata.m_uiSceneInstanceId = bodyInterface.GetUserData(bodyId);

  nsStringBuilder layerName;
  layerName.SetFormat("Layer_{0}", static_cast<nsUInt32>(bodyInterface.GetObjectLayer(bodyId)));
  metadata.m_sLayer = layerName;

  metadata.m_bKinematic = (bodyInterface.GetMotionType(bodyId) == JPH::EMotionType::Kinematic);

  if (auto shape = bodyInterface.GetShape(bodyId))
  {
    const JPH::EShapeSubType subType = shape->GetSubType();
    if (static_cast<nsUInt32>(subType) < JPH::NumSubShapeTypes)
    {
      metadata.m_sShape = JPH::sSubShapeTypeNames[static_cast<nsUInt32>(subType)];
    }
  }
}

void nsJvdRecorder::EnsureClipMetadata()
{
  if (!m_Metadata.m_ClipGuid.IsValid())
  {
    m_Metadata.m_ClipGuid = nsUuid::MakeUuid();
  }

  if (m_Metadata.m_sClipName.IsEmpty())
  {
    nsStringBuilder name;

    nsUInt64 uiLow = 0;
    nsUInt64 uiHigh = 0;
    m_Metadata.m_ClipGuid.GetValues(uiLow, uiHigh);
    name.SetFormat("Recording-{0}-{1}", uiHigh, uiLow);
    m_Metadata.m_sClipName = name;
  }
}

nsResult nsJvdRecorder::SaveClipToFile(nsStringView sFilePath) const
{
  NS_LOCK(m_Mutex);

  if (m_Clip.IsEmpty())
  {
    nsLog::Warning("Cannot save .jvdrec '{0}' because clip is empty.", sFilePath);
    return NS_FAILURE;
  }

  return nsJvdSerialization::SaveClipToFile(sFilePath, m_Clip);
}

NS_STATICLINK_FILE(JVDSDK, Recording_JvdRecorder);
