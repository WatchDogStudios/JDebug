#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

NS_DECLARE_FLAGS(nsUInt8, nsImageConversionFlags, InPlace);

/// \brief Describes a single conversion step between two image formats.
///
/// Used by conversion step implementations to advertise which format pairs they can handle.
/// The conversion system uses this information to build optimal conversion paths.
struct nsImageConversionEntry
{
  nsImageConversionEntry(nsImageFormat::Enum source, nsImageFormat::Enum target, nsImageConversionFlags::Enum flags, float fAdditionalPenalty = 0)
    : m_sourceFormat(source)
    , m_targetFormat(target)
    , m_flags(flags)
    , m_fAdditionalPenalty(fAdditionalPenalty)
  {
  }

  const nsImageFormat::Enum m_sourceFormat;
  const nsImageFormat::Enum m_targetFormat;
  const nsBitflags<nsImageConversionFlags> m_flags;

  /// Additional cost penalty for this conversion step.
  ///
  /// Used to bias the pathfinding algorithm when multiple conversion routes are available.
  /// Higher penalties make this step less likely to be chosen in the optimal path.
  float m_fAdditionalPenalty = 0.0f;
};

/// \brief Interface for a single image conversion step.
///
/// The actual functionality is implemented as either nsImageConversionStepLinear or nsImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class NS_TEXTURE_DLL nsImageConversionStep : public nsEnumerable<nsImageConversionStep>
{
  NS_DECLARE_ENUMERABLE_CLASS(nsImageConversionStep);

protected:
  nsImageConversionStep();
  virtual ~nsImageConversionStep();

public:
  /// \brief Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual nsArrayPtr<const nsImageConversionEntry> GetSupportedConversions() const = 0;
};

/// \brief Interface for a single image conversion step where both the source and target format are uncompressed.
class NS_TEXTURE_DLL nsImageConversionStepLinear : public nsImageConversionStep
{
public:
  /// \brief Converts a batch of pixels.
  virtual nsResult ConvertPixels(nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt64 uiNumElements, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class NS_TEXTURE_DLL nsImageConversionStepDecompressBlocks : public nsImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual nsResult DecompressBlocks(nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt32 uiNumBlocks, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class NS_TEXTURE_DLL nsImageConversionStepCompressBlocks : public nsImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual nsResult CompressBlocks(nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt32 uiNumBlocksX, nsUInt32 uiNumBlocksY,
    nsImageFormat::Enum sourceFormat, nsImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a linear to a planar format.
class NS_TEXTURE_DLL nsImageConversionStepPlanarize : public nsImageConversionStep
{
public:
  /// \brief Converts a batch of pixels into the given target planes.
  virtual nsResult ConvertPixels(const nsImageView& source, nsArrayPtr<nsImage> target, nsUInt32 uiNumPixelsX, nsUInt32 uiNumPixelsY, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a planar to a linear format.
class NS_TEXTURE_DLL nsImageConversionStepDeplanarize : public nsImageConversionStep
{
public:
  /// \brief Converts a batch of pixels from the given source planes.
  virtual nsResult ConvertPixels(nsArrayPtr<nsImageView> source, nsImage target, nsUInt32 uiNumPixelsX, nsUInt32 uiNumPixelsY, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat) const = 0;
};


/// \brief High-level image format conversion system with automatic path finding.
///
/// This class provides a complete image conversion system that can automatically find
/// optimal conversion paths between any two supported formats. It uses a plugin-based
/// architecture where conversion steps register themselves at startup.
///
/// **Basic Usage:**
/// ```cpp
/// // Simple format conversion
/// nsImage sourceImage;
/// sourceImage.LoadFrom("texture.png");
/// nsImage targetImage;
/// nsImageConversion::Convert(sourceImage, targetImage, nsImageFormat::BC1_UNORM);
/// ```
///
/// **Advanced Usage with Path Caching:**
/// ```cpp
/// // Build reusable conversion path
/// nsHybridArray<nsImageConversion::ConversionPathNode, 16> path;
/// nsUInt32 numScratchBuffers;
/// nsImageConversion::BuildPath(sourceFormat, targetFormat, false, path, numScratchBuffers);
///
/// // Use cached path for multiple conversions
/// for (auto& image : images)
/// {
///   nsImageConversion::Convert(image, convertedImage, path, numScratchBuffers);
/// }
/// ```
///
/// The conversion system automatically handles:
/// - Multi-step conversions (e.g., BC1 -> RGBA8 -> BC7)
/// - Memory layout differences (linear, block-compressed, planar)
/// - Optimal path selection based on quality and performance
/// - In-place conversions when possible
class NS_TEXTURE_DLL nsImageConversion
{
public:
  /// \brief Checks if a conversion path exists between two formats.
  ///
  /// This is a fast query that doesn't build the actual conversion path.
  /// Use this to validate format compatibility before attempting conversion.
  static bool IsConvertible(nsImageFormat::Enum sourceFormat, nsImageFormat::Enum targetFormat);

  /// \brief Finds the format requiring the least conversion cost from a list of candidates.
  ///
  /// Useful when you have multiple acceptable target formats and want to choose
  /// the one that preserves the most quality or requires the least processing.
  static nsImageFormat::Enum FindClosestCompatibleFormat(nsImageFormat::Enum format, nsArrayPtr<const nsImageFormat::Enum> compatibleFormats);

  /// \brief A single node along a computed conversion path.
  struct ConversionPathNode
  {
    NS_DECLARE_POD_TYPE();

    const nsImageConversionStep* m_step;
    nsImageFormat::Enum m_sourceFormat;
    nsImageFormat::Enum m_targetFormat;
    nsUInt32 m_sourceBufferIndex;
    nsUInt32 m_targetBufferIndex;
    bool m_inPlace;
  };

  /// \brief Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain
  /// additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not
  ///                               the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work
  ///                               correctly when source and target are the same.
  /// \param out_path               The generated path.
  /// \param out_numScratchBuffers The number of scratch buffers required for the conversion path.
  /// \returns                      ns_SUCCESS if a path was found, ns_FAILURE otherwise.
  static nsResult BuildPath(nsImageFormat::Enum sourceFormat, nsImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
    nsHybridArray<ConversionPathNode, 16>& out_path, nsUInt32& out_uiNumScratchBuffers);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static nsResult Convert(const nsImageView& source, nsImage& ref_target, nsImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static nsResult Convert(const nsImageView& source, nsImage& ref_target, nsArrayPtr<ConversionPathNode> path, nsUInt32 uiNumScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static nsResult ConvertRaw(
    nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt32 uiNumElements, nsImageFormat::Enum sourceFormat, nsImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static nsResult ConvertRaw(
    nsConstByteBlobPtr source, nsByteBlobPtr target, nsUInt32 uiNumElements, nsArrayPtr<ConversionPathNode> path, nsUInt32 uiNumScratchBuffers);

private:
  nsImageConversion();
  nsImageConversion(const nsImageConversion&);

  static nsResult ConvertSingleStep(const nsImageConversionStep* pStep, const nsImageView& source, nsImage& target, nsImageFormat::Enum targetFormat);

  static nsResult ConvertSingleStepDecompress(const nsImageView& source, nsImage& target, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat, const nsImageConversionStep* pStep);

  static nsResult ConvertSingleStepCompress(const nsImageView& source, nsImage& target, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat, const nsImageConversionStep* pStep);

  static nsResult ConvertSingleStepDeplanarize(const nsImageView& source, nsImage& target, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat, const nsImageConversionStep* pStep);

  static nsResult ConvertSingleStepPlanarize(const nsImageView& source, nsImage& target, nsImageFormat::Enum sourceFormat,
    nsImageFormat::Enum targetFormat, const nsImageConversionStep* pStep);

  static void RebuildConversionTable();
};
