#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Serialization/JvdStreamSerializer.h>

#include <Foundation/IO/MemoryStream.h>

namespace
{
  NS_FORCE_INLINE nsResult WriteUuid(nsStreamWriter& stream, const nsUuid& guid)
  {
    nsUInt64 low = 0;
    nsUInt64 high = 0;
    guid.GetValues(low, high);
    if (stream.WriteQWordValue(&low).Failed())
      return NS_FAILURE;
    if (stream.WriteQWordValue(&high).Failed())
      return NS_FAILURE;
    return NS_SUCCESS;
  }

  NS_FORCE_INLINE nsResult ReadUuid(nsStreamReader& stream, nsUuid& guid)
  {
    nsUInt64 low = 0;
    nsUInt64 high = 0;
    if (stream.ReadQWordValue(&low).Failed())
      return NS_FAILURE;
    if (stream.ReadQWordValue(&high).Failed())
      return NS_FAILURE;
    guid = nsUuid(low, high);
    return NS_SUCCESS;
  }

  NS_FORCE_INLINE nsResult WriteVec3(nsStreamWriter& stream, const nsVec3& value)
  {
    return stream.WriteBytes(&value, sizeof(nsVec3));
  }

  NS_FORCE_INLINE nsResult ReadVec3(nsStreamReader& stream, nsVec3& value)
  {
    if (stream.ReadBytes(&value, sizeof(nsVec3)) != sizeof(nsVec3))
      return NS_FAILURE;
    return NS_SUCCESS;
  }

  NS_FORCE_INLINE nsResult WriteQuat(nsStreamWriter& stream, const nsQuat& value)
  {
    return stream.WriteBytes(&value, sizeof(nsQuat));
  }

  NS_FORCE_INLINE nsResult ReadQuat(nsStreamReader& stream, nsQuat& value)
  {
    if (stream.ReadBytes(&value, sizeof(nsQuat)) != sizeof(nsQuat))
      return NS_FAILURE;
    return NS_SUCCESS;
  }
}

nsResult nsJvdSerialization::WriteMetadata(nsStreamWriter& stream, const nsJvdClipMetadata& metadata)
{
  if (WriteUuid(stream, metadata.m_ClipGuid).Failed())
    return NS_FAILURE;

  if (stream.WriteString(metadata.m_sClipName).Failed())
    return NS_FAILURE;
  if (stream.WriteString(metadata.m_sAuthor).Failed())
    return NS_FAILURE;
  if (stream.WriteString(metadata.m_sSourceHost).Failed())
    return NS_FAILURE;

  nsUInt32 uiTagCount = metadata.m_Tags.GetCount();
  if (stream.WriteDWordValue(&uiTagCount).Failed())
    return NS_FAILURE;
  for (nsUInt32 i = 0; i < uiTagCount; ++i)
  {
    if (stream.WriteString(metadata.m_Tags[i]).Failed())
      return NS_FAILURE;
  }

  nsUInt64 creationTicks = static_cast<nsUInt64>(metadata.m_CreationTimeUtc.GetMicroseconds());
  if (stream.WriteQWordValue(&creationTicks).Failed())
    return NS_FAILURE;

  nsUInt64 sampleInterval = static_cast<nsUInt64>(metadata.m_SampleInterval.GetMicroseconds());
  if (stream.WriteQWordValue(&sampleInterval).Failed())
    return NS_FAILURE;

  return NS_SUCCESS;
}

nsResult nsJvdSerialization::ReadMetadata(nsStreamReader& stream, nsJvdClipMetadata& metadata)
{
  metadata.Reset();

  if (ReadUuid(stream, metadata.m_ClipGuid).Failed())
    return NS_FAILURE;

  if (stream.ReadString(metadata.m_sClipName).Failed())
    return NS_FAILURE;
  if (stream.ReadString(metadata.m_sAuthor).Failed())
    return NS_FAILURE;
  if (stream.ReadString(metadata.m_sSourceHost).Failed())
    return NS_FAILURE;

  nsUInt32 uiTagCount = 0;
  if (stream.ReadDWordValue(&uiTagCount).Failed())
    return NS_FAILURE;
  metadata.m_Tags.SetCount(uiTagCount);
  for (nsUInt32 i = 0; i < uiTagCount; ++i)
  {
    if (stream.ReadString(metadata.m_Tags[i]).Failed())
      return NS_FAILURE;
  }

  nsUInt64 creationTicks = 0;
  if (stream.ReadQWordValue(&creationTicks).Failed())
    return NS_FAILURE;
  metadata.m_CreationTimeUtc = nsTime::MakeFromMicroseconds(static_cast<double>(creationTicks));

  nsUInt64 sampleInterval = 0;
  if (stream.ReadQWordValue(&sampleInterval).Failed())
    return NS_FAILURE;
  metadata.m_SampleInterval = nsTime::MakeFromMicroseconds(static_cast<double>(sampleInterval));

  return NS_SUCCESS;
}

nsResult nsJvdSerialization::WriteFrame(nsStreamWriter& stream, const nsJvdFrame& frame)
{
  nsUInt64 frameIndex = frame.m_uiFrameIndex;
  if (stream.WriteQWordValue(&frameIndex).Failed())
    return NS_FAILURE;

  nsUInt64 timestamp = static_cast<nsUInt64>(frame.m_Timestamp.GetMicroseconds());
  if (stream.WriteQWordValue(&timestamp).Failed())
    return NS_FAILURE;

  nsUInt32 bodyCount = frame.m_Bodies.GetCount();
  if (stream.WriteDWordValue(&bodyCount).Failed())
    return NS_FAILURE;

  for (const nsJvdBodyState& state : frame.m_Bodies)
  {
    nsUInt64 bodyId = state.m_uiBodyId;
    if (stream.WriteQWordValue(&bodyId).Failed())
      return NS_FAILURE;

    if (WriteVec3(stream, state.m_vPosition).Failed())
      return NS_FAILURE;
    if (WriteQuat(stream, state.m_qRotation).Failed())
      return NS_FAILURE;
    if (WriteVec3(stream, state.m_vScale).Failed())
      return NS_FAILURE;
    if (WriteVec3(stream, state.m_vLinearVelocity).Failed())
      return NS_FAILURE;
    if (WriteVec3(stream, state.m_vAngularVelocity).Failed())
      return NS_FAILURE;

    if (stream.WriteDWordValue(&state.m_fFriction).Failed())
      return NS_FAILURE;
    if (stream.WriteDWordValue(&state.m_fRestitution).Failed())
      return NS_FAILURE;

    nsUInt8 flags = 0;
    if (state.m_bIsSleeping)
      flags |= 0x01;
    if (state.m_bWasTeleported)
      flags |= 0x02;
    if (stream.WriteBytes(&flags, sizeof(flags)).Failed())
      return NS_FAILURE;

    nsUInt32 customCount = 0;
    if (stream.WriteDWordValue(&customCount).Failed())
      return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsJvdSerialization::ReadFrame(nsStreamReader& stream, nsJvdFrame& frame)
{
  frame.m_Bodies.Clear();

  nsUInt64 frameIndex = 0;
  if (stream.ReadQWordValue(&frameIndex).Failed())
    return NS_FAILURE;
  frame.m_uiFrameIndex = frameIndex;

  nsUInt64 timestamp = 0;
  if (stream.ReadQWordValue(&timestamp).Failed())
    return NS_FAILURE;
  frame.m_Timestamp = nsTime::MakeFromMicroseconds(static_cast<double>(timestamp));

  nsUInt32 bodyCount = 0;
  if (stream.ReadDWordValue(&bodyCount).Failed())
    return NS_FAILURE;

  frame.m_Bodies.Reserve(bodyCount);
  for (nsUInt32 i = 0; i < bodyCount; ++i)
  {
    nsJvdBodyState state;

    nsUInt64 bodyId = 0;
    if (stream.ReadQWordValue(&bodyId).Failed())
      return NS_FAILURE;
    state.m_uiBodyId = bodyId;

    if (ReadVec3(stream, state.m_vPosition).Failed())
      return NS_FAILURE;
    if (ReadQuat(stream, state.m_qRotation).Failed())
      return NS_FAILURE;
    if (ReadVec3(stream, state.m_vScale).Failed())
      return NS_FAILURE;
    if (ReadVec3(stream, state.m_vLinearVelocity).Failed())
      return NS_FAILURE;
    if (ReadVec3(stream, state.m_vAngularVelocity).Failed())
      return NS_FAILURE;

    if (stream.ReadDWordValue(&state.m_fFriction).Failed())
      return NS_FAILURE;
    if (stream.ReadDWordValue(&state.m_fRestitution).Failed())
      return NS_FAILURE;

    nsUInt8 flags = 0;
    if (stream.ReadBytes(&flags, sizeof(flags)) != sizeof(flags))
      return NS_FAILURE;
    state.m_bIsSleeping = (flags & 0x01) != 0;
    state.m_bWasTeleported = (flags & 0x02) != 0;

    nsUInt32 customCount = 0;
    if (stream.ReadDWordValue(&customCount).Failed())
      return NS_FAILURE;
    NS_ASSERT_DEV(customCount == 0, "Custom property serialization not implemented yet");

    frame.m_Bodies.PushBack(std::move(state));
  }

  return NS_SUCCESS;
}

nsResult nsJvdSerialization::WriteClip(nsStreamWriter& stream, const nsJvdClip& clip)
{
  if (WriteMetadata(stream, clip.GetMetadata()).Failed())
    return NS_FAILURE;

  nsUInt64 frameCount = clip.GetFrames().GetCount();
  if (stream.WriteQWordValue(&frameCount).Failed())
    return NS_FAILURE;

  for (const nsJvdFrame& frame : clip.GetFrames())
  {
    if (WriteFrame(stream, frame).Failed())
      return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsJvdSerialization::ReadClip(nsStreamReader& stream, nsJvdClip& clip)
{
  clip.Clear();

  nsJvdClipMetadata metadata;
  if (ReadMetadata(stream, metadata).Failed())
    return NS_FAILURE;

  clip.SetMetadata(metadata);

  nsUInt64 frameCount = 0;
  if (stream.ReadQWordValue(&frameCount).Failed())
    return NS_FAILURE;

  for (nsUInt64 i = 0; i < frameCount; ++i)
  {
    nsJvdFrame frame;
    if (ReadFrame(stream, frame).Failed())
      return NS_FAILURE;
    clip.AddFrame(std::move(frame));
  }

  return NS_SUCCESS;
}

NS_STATICLINK_FILE(JVDSDK, Serialization_JvdStreamSerializer);
