#pragma once

#include <JVDSDK/JVDSDKDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>


namespace nsJvdIds
{
  constexpr nsUInt32 g_uiTelemetrySystemId = 0x4A564420; // 'JVD '
  constexpr nsUInt32 g_uiTelemetryFrameMessageId = 0x6672616D; // 'fram'
  constexpr nsUInt32 g_uiTelemetryCommandMessageId = 0x636D6473; // 'cmds'
  constexpr nsUInt32 g_uiTelemetryClipMessageId = 0x636C6970;   // 'clip'
}

struct NS_JVDSDK_DLL nsJvdBodyMetadata
{
  nsUuid m_BodyGuid;
  nsUInt64 m_uiBodyId = 0;
  nsUInt64 m_uiSceneInstanceId = 0;
  nsString m_sName;
  nsString m_sLayer;
  nsString m_sShape;
  nsString m_sMaterial;
  bool m_bKinematic = false;
  bool m_bTrigger = false;

  void Reset();

  const nsUuid& GetBodyGuid() const { return m_BodyGuid; }
  void SetBodyGuid(const nsUuid& guid) { m_BodyGuid = guid; }
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdBodyMetadata);

struct NS_JVDSDK_DLL nsJvdBodyState
{
  nsUInt64 m_uiBodyId = 0;
  nsVec3 m_vPosition = nsVec3::MakeZero();
  nsQuat m_qRotation = nsQuat::MakeIdentity();
  nsVec3 m_vScale = nsVec3(1.0f);
  nsVec3 m_vLinearVelocity = nsVec3::MakeZero();
  nsVec3 m_vAngularVelocity = nsVec3::MakeZero();
  float m_fFriction = 0.0f;
  float m_fRestitution = 0.0f;
  bool m_bIsSleeping = false;
  bool m_bWasTeleported = false;
  nsVariantDictionary m_CustomProperties;

  void Reset();
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdBodyState);

struct NS_JVDSDK_DLL nsJvdFrame
{
  nsUInt64 m_uiFrameIndex = 0;
  nsTime m_Timestamp = nsTime::MakeZero();
  nsDynamicArray<nsJvdBodyState> m_Bodies;

  const nsJvdBodyState* FindBody(nsUInt64 uiBodyId) const;
  nsJvdBodyState* FindBody(nsUInt64 uiBodyId);
  void AddOrUpdateBody(const nsJvdBodyState& state);
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdFrame);

struct NS_JVDSDK_DLL nsJvdClipMetadata
{
  nsUuid m_ClipGuid;
  nsString m_sClipName;
  nsString m_sAuthor;
  nsString m_sSourceHost;
  nsHybridArray<nsString, 8> m_Tags;
  nsTime m_CreationTimeUtc = nsTime::MakeZero();
  nsTime m_SampleInterval = nsTime::MakeZero();

  void Reset();
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdClipMetadata);

class NS_JVDSDK_DLL nsJvdClip
{
public:
  nsJvdClip();
  nsJvdClip(const nsJvdClip& other);
  nsJvdClip(nsJvdClip&& other) noexcept;
  ~nsJvdClip();

  nsJvdClip& operator=(const nsJvdClip& other);
  nsJvdClip& operator=(nsJvdClip&& other) noexcept;

  void Clear();

  void SetMetadata(const nsJvdClipMetadata& metadata);
  const nsJvdClipMetadata& GetMetadata() const;

  nsUInt64 AddFrame(nsJvdFrame&& frame);
  const nsDynamicArray<nsJvdFrame>& GetFrames() const { return m_Frames; }
  nsDynamicArray<nsJvdFrame>& GetFrames() { return m_Frames; }

  const nsJvdFrame* FindFrameByTime(nsTime timestamp) const;
  const nsJvdFrame* FindFrame(nsUInt64 uiFrameIndex) const;

  bool IsEmpty() const { return m_Frames.IsEmpty(); }

  nsTime GetDuration() const;
  nsTime GetSampleInterval() const;

private:
  NS_ALLOW_PRIVATE_PROPERTIES(nsJvdClip);

  nsJvdClipMetadata m_Metadata;
  nsDynamicArray<nsJvdFrame> m_Frames;
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdClip);

struct NS_JVDSDK_DLL nsJvdRecordingSettings
{
  nsString m_sSessionName;
  nsHybridArray<nsUInt64, 32> m_IncludedBodies;
  nsHybridArray<nsUInt64, 32> m_ExcludedBodies;
  nsTime m_TargetFrameInterval = nsTime::MakeFromSeconds(1.0 / 60.0);
  nsTime m_MaximumCaptureTime = nsTime::MakeZero();
  bool m_bCaptureSleepingBodies = false;
  bool m_bRecordVelocities = true;
  bool m_bRecordCustomProperties = false;

  void Reset();
};
NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdRecordingSettings);
