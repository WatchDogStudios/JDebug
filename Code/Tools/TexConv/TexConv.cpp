#include <TexConv/TexConvPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/nsTexFormat/nsTexFormat.h>

nsTexConv::nsTexConv()
  : nsApplication("TexConv")
{
}

nsResult nsTexConv::BeforeCoreSystemsStartup()
{
  nsStartup::AddApplicationTag("tool");
  nsStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void nsTexConv::AfterCoreSystemsStartup()
{
  nsFileSystem::AddDataDirectory("", "App", ":", nsDataDirUsage::AllowWrites).IgnoreResult();

  nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
}

void nsTexConv::BeforeCoreSystemsShutdown()
{
  nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

nsResult nsTexConv::DetectOutputFormat()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = nsTexConvOutputType::None;
    return NS_SUCCESS;
  }

  nsStringBuilder sExt = nsPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = true;
    return NS_SUCCESS;
  }
  if (sExt == "TGA" || sExt == "PNG")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return NS_SUCCESS;
  }
  if (sExt == "NSBINTEXTURE2D")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return NS_SUCCESS;
  }
  if (sExt == "NSBINTEXTURE3D")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return NS_SUCCESS;
  }
  if (sExt == "NSBINTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return NS_SUCCESS;
  }
  if (sExt == "NSBINTEXTUREATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = true;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return NS_SUCCESS;
  }
  if (sExt == "NSBINIMAGEDATA")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return NS_SUCCESS;
  }

  nsLog::Error("Output file uses unsupported file format '{}'", sExt);
  return NS_FAILURE;
}

bool nsTexConv::IsTexFormat() const
{
  const nsStringView ext = nsPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("ns");
}

nsResult nsTexConv::WriteTexFile(nsStreamWriter& inout_stream, const nsImage& image)
{
  nsAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  NS_SUCCEED_OR_RETURN(asset.Write(inout_stream));

  nsTexFormat texFormat;
  texFormat.m_bSRGB = nsImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(inout_stream);

  nsDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(inout_stream, image, "dds").Failed())
  {
    nsLog::Error("Failed to write DDS image chunk to nsTex file.");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsTexConv::WriteOutputFile(nsStringView sFile, const nsImage& image)
{
  if (sFile.HasExtension("nsBinImageData"))
  {
    nsDeferredFileWriter file;
    file.SetOutput(sFile);

    nsAssetFileHeader asset;
    asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    if (asset.Write(file).Failed())
    {
      nsLog::Error("Failed to write asset header to file.");
      return NS_FAILURE;
    }

    nsUInt8 uiVersion = 1;
    file << uiVersion;

    nsUInt8 uiFormat = 1; // 1 == PNG
    file << uiFormat;

    nsStbImageFileFormats pngWriter;
    if (pngWriter.WriteImage(file, image, "png").Failed())
    {
      nsLog::Error("Failed to write data as PNG to nsImageData file.");
      return NS_FAILURE;
    }

    return file.Close();
  }
  else if (IsTexFormat())
  {
    nsDeferredFileWriter file;
    file.SetOutput(sFile);

    NS_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(sFile);
  }
}

void nsTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  if (m_Mode == nsTexConvMode::Compare)
  {
    if (m_Comparer.Compare().Failed())
    {
      RequestApplicationQuit();
      return;
    }

    SetReturnCode(0);

    if (m_Comparer.m_bExceededMSE)
    {
      SetReturnCode(m_Comparer.m_OutputMSE);

      if (!m_sOutputFile.IsEmpty())
      {
        nsStringBuilder tmp;

        tmp.Set(m_sOutputFile, "-rgb.png");
        m_Comparer.m_OutputImageDiffRgb.SaveTo(tmp).IgnoreResult();

        tmp.Set(m_sOutputFile, "-alpha.png");
        m_Comparer.m_OutputImageDiffAlpha.SaveTo(tmp).IgnoreResult();

        if (!m_sHtmlTitle.IsEmpty())
        {
          tmp.Set(m_sOutputFile, ".htm");

          nsFileWriter file;
          if (file.Open(tmp).Succeeded())
          {
            nsStringBuilder html;

            nsImageUtils::CreateImageDiffHtml(html, m_sHtmlTitle, m_Comparer.m_ExtractedExpectedRgb, m_Comparer.m_ExtractedExpectedAlpha, m_Comparer.m_ExtractedActualRgb, m_Comparer.m_ExtractedActualAlpha, m_Comparer.m_OutputImageDiffRgb, m_Comparer.m_OutputImageDiffAlpha, m_Comparer.m_OutputMSE, m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold, m_Comparer.m_uiOutputMinDiffRgb, m_Comparer.m_uiOutputMaxDiffRgb, m_Comparer.m_uiOutputMinDiffAlpha, m_Comparer.m_uiOutputMaxDiffAlpha);

            file.WriteBytes(html.GetData(), html.GetElementCount()).AssertSuccess();
          }
        }
      }
    }
  }
  else
  {
    if (m_Processor.Process().Failed())
    {
      RequestApplicationQuit();
      return;
    }

    if (m_Processor.m_Descriptor.m_OutputType == nsTexConvOutputType::Atlas)
    {
      nsDeferredFileWriter file;
      file.SetOutput(m_sOutputFile);

      nsAssetFileHeader header;
      header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

      header.Write(file).IgnoreResult();

      m_Processor.m_TextureAtlas.CopyToStream(file).IgnoreResult();

      if (file.Close().Succeeded())
      {
        SetReturnCode(0);
      }
      else
      {
        nsLog::Error("Failed to write atlas output image.");
      }

      RequestApplicationQuit();
      return;
    }

    if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
      {
        nsLog::Error("Failed to write main result to '{}'", m_sOutputFile);
        RequestApplicationQuit();
        return;
      }

      nsLog::Success("Wrote main result to '{}'", m_sOutputFile);
    }

    if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
    {
      if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
      {
        nsLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
        RequestApplicationQuit();
        return;
      }

      nsLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
    }

    if (!m_sOutputLowResFile.IsEmpty())
    {
      // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
      nsOSFile::DeleteFile(m_sOutputLowResFile).IgnoreResult();

      if (m_Processor.m_LowResOutputImage.IsValid())
      {
        if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
        {
          nsLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
          RequestApplicationQuit();
          return;
        }

        nsLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
      }
    }

    SetReturnCode(0);
  }

  RequestApplicationQuit();
}

NS_APPLICATION_ENTRY_POINT(nsTexConv);
