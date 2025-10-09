#include <Texture/TexturePCH.h>

#if NS_USE_BC7ENC

#  include <bc7enc_rdo/rdo_bc_encoder.h>

#  include <Foundation/System/SystemInformation.h>
#  include <Texture/Image/ImageConversion.h>

nsImageConversionEntry g_BC7EncConversions[] = {
  // Even at the lowest quality level of BC7Enc, BC1 encoding times are more than a magnitude worse than DXTexConv.
  // nsImageConversionEntry(nsImageFormat::R8G8B8A8_UNORM, nsImageFormat::BC1_UNORM, nsImageConversionFlags::Default),
  // nsImageConversionEntry(nsImageFormat::R8G8B8A8_UNORM_SRGB, nsImageFormat::BC1_UNORM_SRGB, nsImageConversionFlags::Default),
  nsImageConversionEntry(nsImageFormat::R8G8B8A8_UNORM, nsImageFormat::BC7_UNORM, nsImageConversionFlags::Default),
  nsImageConversionEntry(nsImageFormat::R8G8B8A8_UNORM_SRGB, nsImageFormat::BC7_UNORM_SRGB, nsImageConversionFlags::Default),
};

class nsImageConversion_CompressBC7Enc : public nsImageConversionStepCompressBlocks
{
public:
  virtual nsArrayPtr<const nsImageConversionEntry> GetSupportedConversions() const override
  {
    return g_BC7EncConversions;
  }

  virtual nsResult CompressBlocks(nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt32 numBlocksX, nsUInt32 numBlocksY,
    nsImageFormat::Enum sourceFormat, nsImageFormat::Enum targetFormat) const override
  {
    nsSystemInformation info = nsSystemInformation::Get();
    const nsInt32 iCpuCores = info.GetCPUCoreCount();

    rdo_bc::rdo_bc_params rp;
    rp.m_rdo_max_threads = nsMath::Clamp<nsInt32>(iCpuCores - 2, 2, 8);
    rp.m_status_output = false;
    rp.m_bc1_quality_level = 18;

    switch (targetFormat)
    {
      case nsImageFormat::BC7_UNORM:
      case nsImageFormat::BC7_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC7_UNORM;
        break;
      case nsImageFormat::BC1_UNORM:
      case nsImageFormat::BC1_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC1_UNORM;
        break;
      default:
        NS_ASSERT_NOT_IMPLEMENTED;
    }

    utils::image_u8 source_image(numBlocksX * 4, numBlocksY * 4);
    auto& pixels = source_image.get_pixels();
    nsMemoryUtils::Copy<nsUInt32>(reinterpret_cast<nsUInt32*>(pixels.data()), reinterpret_cast<const nsUInt32*>(source.GetPtr()), numBlocksX * 4 * numBlocksY * 4);

    rdo_bc::rdo_bc_encoder encoder;
    if (!encoder.init(source_image, rp))
    {
      nsLog::Error("rdo_bc_encoder::init() failed!");
      return NS_FAILURE;
    }

    if (!encoder.encode())
    {
      nsLog::Error("rdo_bc_encoder::encode() failed!");
      return NS_FAILURE;
    }

    const nsUInt32 uiTotalBytes = encoder.get_total_blocks_size_in_bytes();
    if (uiTotalBytes != target.GetCount())
    {
      nsLog::Error("Encoder output of {} byte does not match the expected size of {} bytes", uiTotalBytes, target.GetCount());
      return NS_FAILURE;
    }
    nsMemoryUtils::Copy<nsUInt8>(reinterpret_cast<nsUInt8*>(target.GetPtr()), reinterpret_cast<const nsUInt8*>(encoder.get_blocks()), uiTotalBytes);
    return NS_SUCCESS;
  }
};

NS_STATICLINK_FORCE static nsImageConversion_CompressBC7Enc s_conversion_compressBC7Enc;

#endif

NS_STATICLINK_FILE(Texture, Texture_Image_Conversions_BC7EncConversions);
