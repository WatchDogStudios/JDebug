#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

/// \brief Describes how to create a texture atlas from multiple input images.
///
/// A texture atlas packs multiple images into a single texture for improved rendering performance.
/// This structure defines the layers (different image types like diffuse, normal maps) and items
/// (individual images to pack) that will be combined.
///
/// **Workflow:**
/// 1. Create nsTextureAtlasCreationDesc with desired layers and items
/// 2. Use texture conversion system to generate the actual atlas
/// 3. Load the resulting nsTextureAtlasRuntimeDesc for runtime access
///
/// **Example:**
/// ```cpp
/// nsTextureAtlasCreationDesc desc;
///
/// // Add a diffuse layer
/// auto& diffuseLayer = desc.m_Layers.ExpandAndGetRef();
/// diffuseLayer.m_Usage = nsTexConvUsage::Color;
/// diffuseLayer.m_uiNumChannels = 4;
///
/// // Add an item (sprite/icon)
/// auto& item = desc.m_Items.ExpandAndGetRef();
/// item.m_uiUniqueID = 100;
/// item.m_sLayerInput[0] = "icon_sword.png";
/// ```
struct NS_TEXTURE_DLL nsTextureAtlasCreationDesc
{
  /// \brief Defines a single layer in the texture atlas (e.g., diffuse, normal, roughness).
  struct Layer
  {
    nsEnum<nsTexConvUsage> m_Usage;
    nsUInt8 m_uiNumChannels = 4;
  };

  /// \brief Represents one item (image) to be packed into the atlas.
  struct Item
  {
    nsUInt32 m_uiUniqueID;
    nsUInt32 m_uiFlags;
    nsString m_sAlphaInput;
    nsString m_sLayerInput[4];
  };

  nsHybridArray<Layer, 4> m_Layers;
  nsDynamicArray<Item> m_Items;

  nsResult Serialize(nsStreamWriter& inout_stream) const;
  nsResult Deserialize(nsStreamReader& inout_stream);

  nsResult Save(nsStringView sFile) const;
  nsResult Load(nsStringView sFile);
};

/// \brief Runtime data for efficiently accessing items within a generated texture atlas.
///
/// After a texture atlas is created and processed, this structure provides the information
/// needed to find and render individual items from the packed atlas texture. It maps unique
/// item IDs to their rectangular regions within each layer of the atlas.
///
/// **Runtime Usage:**
/// ```cpp
/// nsTextureAtlasRuntimeDesc atlas;
/// atlas.Load("MyAtlas.nsAtlas");
///
/// // Find an item by ID
/// auto item = atlas.m_Items.Find(itemID);
/// if (item.IsValid())
/// {
///   // Get UV coordinates for diffuse layer (layer 0)
///   nsRectU32 rect = item.Value().m_LayerRects[0];
///   // Convert to UV coordinates based on atlas texture size
/// }
/// ```
struct NS_TEXTURE_DLL nsTextureAtlasRuntimeDesc
{
  struct Item
  {
    nsUInt32 m_uiFlags;
    nsRectU32 m_LayerRects[4];
  };

  nsUInt32 m_uiNumLayers = 0;
  nsArrayMap<nsUInt32, Item> m_Items;

  void Clear();

  nsResult Serialize(nsStreamWriter& inout_stream) const;
  nsResult Deserialize(nsStreamReader& inout_stream);
};
