#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

struct stbrp_node;
struct stbrp_rect;

class NS_TEXTURE_DLL nsTexturePacker
{
  NS_DISALLOW_COPY_AND_ASSIGN(nsTexturePacker);

public:
  struct Texture
  {
    NS_DECLARE_POD_TYPE();

    nsVec2U32 m_Size;
    nsVec2U32 m_Position;
  };

  nsTexturePacker();
  ~nsTexturePacker();

  void SetTextureSize(nsUInt32 uiWidth, nsUInt32 uiHeight, nsUInt32 uiReserveTextures = 0);

  void AddTexture(nsUInt32 uiWidth, nsUInt32 uiHeight);

  const nsDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  nsResult PackTextures();

private:
  nsUInt32 m_uiWidth = 0;
  nsUInt32 m_uiHeight = 0;

  nsDynamicArray<Texture> m_Textures;

  nsDynamicArray<stbrp_node> m_Nodes;
  nsDynamicArray<stbrp_rect> m_Rects;
};
