#pragma once

#include <JVDSDK/Recording/JvdConversion.h>
#include <JVDSDK/Recording/JvdRecordingTypes.h>

#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Threading/Mutex.h>

namespace JPH
{
  class PhysicsSystem;
  class BodyInterface;
  class BodyID;
} // namespace JPH

class NS_JVDSDK_DLL nsJvdRecorder
{
public:
  nsJvdRecorder();
  ~nsJvdRecorder();

  void StartRecording(const nsJvdRecordingSettings& settings, const nsJvdClipMetadata& metadata = {});
  nsResult StopRecording(nsJvdClip& out_clip);
  void CancelRecording();

  bool IsRecording() const { return m_bRecording; }

  const nsJvdRecordingSettings& GetSettings() const { return m_Settings; }
  const nsJvdClipMetadata& GetMetadata() const { return m_Metadata; }

  void SetMetadata(const nsJvdClipMetadata& metadata);

  /// brief Appends a new frame constructed from the provided body states.
  void AppendFrame(nsTime timestamp, nsArrayPtr<const nsJvdBodyState> states);

  /// brief Captures the state of all bodies currently in the provided Jolt physics system.
  nsResult CapturePhysicsSystem(const JPH::PhysicsSystem& physicsSystem, nsTime timestamp);

  /// brief Captures the state of a list of bodies fetched through the supplied body interface.
  nsResult CaptureBodies(const JPH::BodyInterface& bodyInterface, nsArrayPtr<const JPH::BodyID> bodyIds, nsTime timestamp);

  /// brief Returns the currently accumulated clip without stopping the recording.
  const nsJvdClip& PeekClip() const { return m_Clip; }
  const nsMap<nsUInt64, nsJvdBodyMetadata>& GetBodyMetadata() const { return m_BodyMetadata; }

  /// \brief Saves the currently recorded clip to a .jvdrec file.
  nsResult SaveClipToFile(nsStringView sFilePath) const;

private:
  bool ShouldCaptureBody(nsUInt64 uiBodyId, bool bIsSleeping) const;
  void UpdateBodyMetadata(const JPH::BodyInterface& bodyInterface, const JPH::BodyID& bodyId);
  void EnsureClipMetadata();

  mutable nsMutex m_Mutex;
  bool m_bRecording = false;
  nsTime m_StartTime = nsTime::MakeZero();
  nsTime m_LastSampleTime = nsTime::MakeZero();
  nsJvdRecordingSettings m_Settings;
  nsJvdClipMetadata m_Metadata;
  nsJvdClip m_Clip;
  nsMap<nsUInt64, nsJvdBodyMetadata> m_BodyMetadata;
};
