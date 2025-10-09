#pragma once

#include <JVDSDK/Recording/JvdRecordingTypes.h>

namespace nsJvdSerialization
{
  NS_JVDSDK_DLL nsResult SaveClipToFile(nsStringView sFilePath, const nsJvdClip& clip);
  NS_JVDSDK_DLL nsResult LoadClipFromFile(nsStringView sFilePath, nsJvdClip& outClip);
}
