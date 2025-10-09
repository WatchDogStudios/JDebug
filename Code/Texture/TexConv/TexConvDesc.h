#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct nsTexConvChannelMapping
{
  nsInt8 m_iInputImageIndex = -1;
  nsTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct nsTexConvSliceChannelMapping
{
  nsTexConvChannelMapping m_Channel[4] = {
    nsTexConvChannelMapping{-1, nsTexConvChannelValue::Red},
    nsTexConvChannelMapping{-1, nsTexConvChannelValue::Green},
    nsTexConvChannelMapping{-1, nsTexConvChannelValue::Blue},
    nsTexConvChannelMapping{-1, nsTexConvChannelValue::Alpha},
  };
};

/// \brief Complete texture conversion configuration with all processing options.
///
/// This structure contains all settings needed to convert source images into optimized
/// textures for runtime use. It handles input specification, format conversion, quality
/// settings, mipmap generation, and platform-specific optimizations.
///
/// **Basic Usage Pattern:**
/// ```cpp
/// nsTexConvDesc desc;
/// desc.m_InputFiles.PushBack("diffuse.png");
/// desc.m_OutputType = nsTexConvOutputType::Texture2D;
/// desc.m_Usage = nsTexConvUsage::Color;
/// desc.m_CompressionMode = nsTexConvCompressionMode::HighQuality;
/// desc.m_TargetPlatform = nsTexConvTargetPlatform::PC;
/// // Process with nsTexConvProcessor...
/// ```
class NS_TEXTURE_DLL nsTexConvDesc
{
  NS_DISALLOW_COPY_AND_ASSIGN(nsTexConvDesc);

public:
  nsTexConvDesc() = default;

  // Input specification
  nsHybridArray<nsString, 4> m_InputFiles;                          ///< Source image file paths to process
  nsDynamicArray<nsImage> m_InputImages;                            ///< Pre-loaded source images (alternative to file paths)

  nsHybridArray<nsTexConvSliceChannelMapping, 6> m_ChannelMappings; ///< Channel routing for multi-input processing

  // Output configuration
  nsEnum<nsTexConvOutputType> m_OutputType;         ///< Type of texture to generate (2D, Cubemap, 3D, etc.)
  nsEnum<nsTexConvTargetPlatform> m_TargetPlatform; ///< Target platform for format optimization

  // Multi-resolution output
  nsUInt32 m_uiLowResMipmaps = 0;             ///< Number of low-resolution mipmap levels to generate separately
  nsUInt32 m_uiThumbnailOutputResolution = 0; ///< Size for thumbnail generation (0 = no thumbnail)

  // Format and compression
  nsEnum<nsTexConvUsage> m_Usage;                     ///< Intended usage (Color, Normal, Linear, etc.) affects format selection
  nsEnum<nsTexConvCompressionMode> m_CompressionMode; ///< Quality vs file size trade-off

  // Resolution control
  nsUInt32 m_uiMinResolution = 16;       ///< Minimum texture dimension (prevents over-downscaling)
  nsUInt32 m_uiMaxResolution = 1024 * 8; ///< Maximum texture dimension (prevents excessive memory usage)
  nsUInt32 m_uiDownscaleSteps = 0;       ///< Number of 2x downscaling steps to apply

  // Mipmap generation
  nsEnum<nsTexConvMipmapMode> m_MipmapMode;    ///< Mipmap generation strategy
  nsEnum<nsTextureFilterSetting> m_FilterMode; ///< Runtime filtering quality (ns formats only)
  nsEnum<nsImageAddressMode> m_AddressModeU;   ///< Horizontal texture wrapping mode
  nsEnum<nsImageAddressMode> m_AddressModeV;   ///< Vertical texture wrapping mode
  nsEnum<nsImageAddressMode> m_AddressModeW;   ///< Depth texture wrapping mode (3D textures)
  bool m_bPreserveMipmapCoverage = false;      ///< Maintain alpha coverage for alpha testing
  float m_fMipmapAlphaThreshold = 0.5f;        ///< Alpha threshold for coverage preservation

  // Image processing options
  nsUInt8 m_uiDilateColor = 0;      ///< Color dilation steps (fills transparent areas)
  bool m_bFlipHorizontal = false;   ///< Mirror image horizontally
  bool m_bPremultiplyAlpha = false; ///< Pre-multiply RGB by alpha for correct blending
  float m_fHdrExposureBias = 0.0f;  ///< HDR exposure adjustment (stops)
  float m_fMaxValue = 64000.f;      ///< HDR value clamping

  // Runtime metadata
  nsUInt64 m_uiAssetHash = 0;    ///< Content hash for cache invalidation
  nsUInt16 m_uiAssetVersion = 0; ///< Asset version for dependency tracking

  // Advanced features
  nsString m_sTextureAtlasDescFile;               ///< Path to texture atlas description file
  nsEnum<nsTexConvBumpMapFilter> m_BumpMapFilter; ///< Bump map specific filtering
};
