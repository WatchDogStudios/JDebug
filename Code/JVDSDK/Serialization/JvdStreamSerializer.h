#pragma once

#include <JVDSDK/Recording/JvdRecordingTypes.h>

#include <Foundation/IO/Stream.h>

namespace nsJvdSerialization
{
  NS_JVDSDK_DLL nsResult WriteMetadata(nsStreamWriter& stream, const nsJvdClipMetadata& metadata);
  NS_JVDSDK_DLL nsResult ReadMetadata(nsStreamReader& stream, nsJvdClipMetadata& metadata);

  NS_JVDSDK_DLL nsResult WriteFrame(nsStreamWriter& stream, const nsJvdFrame& frame);
  NS_JVDSDK_DLL nsResult ReadFrame(nsStreamReader& stream, nsJvdFrame& frame);

  NS_JVDSDK_DLL nsResult WriteClip(nsStreamWriter& stream, const nsJvdClip& clip);
  NS_JVDSDK_DLL nsResult ReadClip(nsStreamReader& stream, nsJvdClip& clip);
}
