#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Serialization/JvdFileIO.h>
#include <JVDSDK/Serialization/JvdStreamSerializer.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryUtils.h>

namespace
{
  constexpr nsUInt8 g_szJvdMagic[] = {'J', 'V', 'D', 'R', 'E', 'C'};
  constexpr nsUInt32 g_uiJvdVersion = 1;
}

nsResult nsJvdSerialization::SaveClipToFile(nsStringView sFilePath, const nsJvdClip& clip)
{
  nsFileWriter file;
  if (file.Open(sFilePath).Failed())
  {
    nsLog::Error("Failed to open '{0}' for writing .jvdrec clip.", sFilePath);
    return NS_FAILURE;
  }

  if (file.WriteBytes(g_szJvdMagic, sizeof(g_szJvdMagic)).Failed())
    return NS_FAILURE;

  nsUInt32 uiVersion = g_uiJvdVersion;
  if (file.WriteDWordValue(&uiVersion).Failed())
    return NS_FAILURE;

  if (WriteClip(file, clip).Failed())
  {
    nsLog::Error("Failed to serialize clip to '{0}'.", sFilePath);
    return NS_FAILURE;
  }

  file.Flush().IgnoreResult();
  return NS_SUCCESS;
}

nsResult nsJvdSerialization::LoadClipFromFile(nsStringView sFilePath, nsJvdClip& outClip)
{
  nsFileReader file;
  if (file.Open(sFilePath).Failed())
  {
    nsLog::Error("Failed to open '{0}' for reading .jvdrec clip.", sFilePath);
    return NS_FAILURE;
  }

  nsUInt8 header[sizeof(g_szJvdMagic)] = {};
  if (file.ReadBytes(header, sizeof(header)) != sizeof(header))
  {
    nsLog::Error("File '{0}' is too small to be a valid .jvdrec.", sFilePath);
    return NS_FAILURE;
  }

  if (!nsMemoryUtils::IsEqual(header, g_szJvdMagic, sizeof(g_szJvdMagic)))
  {
    nsLog::Error("File '{0}' has invalid .jvdrec header.", sFilePath);
    return NS_FAILURE;
  }

  nsUInt32 uiVersion = 0;
  if (file.ReadDWordValue(&uiVersion).Failed())
  {
    nsLog::Error("File '{0}' missing version information.", sFilePath);
    return NS_FAILURE;
  }

  if (uiVersion != g_uiJvdVersion)
  {
    nsLog::Warning("Loading .jvdrec version {0}, expected {1}. Attempting to continue.", uiVersion, g_uiJvdVersion);
  }

  if (ReadClip(file, outClip).Failed())
  {
    nsLog::Error("Failed to deserialize clip from '{0}'.", sFilePath);
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

NS_STATICLINK_FILE(JVDSDK, Serialization_JvdFileIO);
