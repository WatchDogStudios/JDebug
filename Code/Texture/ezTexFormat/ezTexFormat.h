#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class nsStreamWriter;
class nsStreamReader;

/// \brief Runtime texture format metadata stored in nsTex files.
///
/// This structure contains all the metadata needed by the renderer to properly sample
/// and use textures at runtime. It's saved as a header in .nsTex files and loaded
/// by the texture resource system.
struct NS_TEXTURE_DLL nsTexFormat
{
  bool m_bSRGB = false;
  nsEnum<nsImageAddressMode> m_AddressModeU;
  nsEnum<nsImageAddressMode> m_AddressModeV;
  nsEnum<nsImageAddressMode> m_AddressModeW;

  // version 2
  nsEnum<nsTextureFilterSetting> m_TextureFilter;

  // Version 3 additions - Render target specific
  nsInt16 m_iRenderTargetResolutionX = 0; ///< Fixed render target width (0 = dynamic)
  nsInt16 m_iRenderTargetResolutionY = 0; ///< Fixed render target height (0 = dynamic)

  // Version 4 additions
  float m_fResolutionScale = 1.0f; ///< Resolution scaling factor for dynamic render targets

  // Version 5 additions
  int m_GalRenderTargetFormat = 0; ///< Graphics abstraction layer format for render targets

  void WriteTextureHeader(nsStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(nsStreamWriter& inout_stream) const;
  void ReadHeader(nsStreamReader& inout_stream);
};
