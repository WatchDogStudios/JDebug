#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

const nsImageFileFormat* nsImageFileFormat::GetReaderFormat(nsStringView sExtension)
{
  for (auto format = nsRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanReadFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

const nsImageFileFormat* nsImageFileFormat::GetWriterFormat(nsStringView sExtension)
{
  for (auto format = nsRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanWriteFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

nsResult nsImageFileFormat::ReadImageHeader(nsStringView sFileName, nsImageHeader& ref_header)
{
  NS_LOG_BLOCK("Read Image Header", sFileName);

  NS_PROFILE_SCOPE(nsPathUtils::GetFileNameAndExtension(sFileName));

  nsFileReader reader;
  if (reader.Open(sFileName) == NS_FAILURE)
  {
    nsLog::Warning("Failed to open image file '{0}'", nsArgSensitive(sFileName, "File"));
    return NS_FAILURE;
  }

  nsStringView it = nsPathUtils::GetFileExtension(sFileName);

  if (const nsImageFileFormat* pFormat = nsImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it) != NS_SUCCESS)
    {
      nsLog::Warning("Failed to read image file '{0}'", nsArgSensitive(sFileName, "File"));
      return NS_FAILURE;
    }

    return NS_SUCCESS;
  }

  nsLog::Warning("No known image file format for extension '{0}'", it);
  return NS_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

NS_ENUMERABLE_CLASS_IMPLEMENTATION(nsRegisteredImageFileFormat);

nsRegisteredImageFileFormat::nsRegisteredImageFileFormat() = default;
nsRegisteredImageFileFormat::~nsRegisteredImageFileFormat() = default;
