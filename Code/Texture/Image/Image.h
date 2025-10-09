#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// \brief A lightweight view to image data without owning the memory.
///
/// nsImageView provides read-only access to image data along with the metadata needed to interpret it.
/// It does not own the image data, so the underlying memory must remain valid for the lifetime of the view.
/// This class is ideal for passing image data around without unnecessary copying.
///
/// Use cases:
/// - Passing images to functions that only read data
/// - Creating temporary views to sub-regions of larger images
/// - Interfacing with external image processing libraries
/// - Converting between different image representations
class NS_TEXTURE_DLL nsImageView : protected nsImageHeader
{
public:
  /// \brief Constructs an empty image view.
  nsImageView();

  /// \brief Constructs an image view with the given header and image data.
  nsImageView(const nsImageHeader& header, nsConstByteBlobPtr imageData);

  /// \brief Resets to an empty state, releasing the reference to external data.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Resets the view to reference new external image data.
  ///
  /// Any previous data reference is released. The new data must remain valid
  /// for the lifetime of this view.
  void ResetAndViewExternalStorage(const nsImageHeader& header, nsConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  nsResult SaveTo(nsStringView sFileName) const;

  /// \brief Returns the header this image was constructed from.
  const nsImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  nsBlobPtr<const T> GetBlobPtr() const;

  nsConstByteBlobPtr GetByteBlobPtr() const;

  /// \brief Returns a view to the given sub-image.
  nsImageView GetSubImageView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to a sub-plane.
  nsImageView GetPlaneView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  nsImageView GetSliceView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  nsImageView GetRowView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 y = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 x = 0, nsUInt32 y = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(nsImageFormat::Enum format);

public:
  using nsImageHeader::GetDepth;
  using nsImageHeader::GetHeight;
  using nsImageHeader::GetWidth;

  using nsImageHeader::GetNumArrayIndices;
  using nsImageHeader::GetNumFaces;
  using nsImageHeader::GetNumMipLevels;
  using nsImageHeader::GetPlaneCount;

  using nsImageHeader::GetImageFormat;

  using nsImageHeader::GetNumBlocksX;
  using nsImageHeader::GetNumBlocksY;
  using nsImageHeader::GetNumBlocksZ;

  using nsImageHeader::GetDepthPitch;
  using nsImageHeader::GetRowPitch;

protected:
  nsUInt64 ComputeLayout();

  void ValidateSubImageIndices(nsUInt32 uiMipLevel, nsUInt32 uiFace, nsUInt32 uiArrayIndex, nsUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(nsUInt32 uiPlaneIndex) const;

  const nsUInt64& GetSubImageOffset(nsUInt32 uiMipLevel, nsUInt32 uiFace, nsUInt32 uiArrayIndex, nsUInt32 uiPlaneIndex) const;

  nsHybridArray<nsUInt64, 16> m_SubImageOffsets;
  nsBlobPtr<nsUInt8> m_DataPtr;
};

/// \brief Container for image data with automatic memory management.
///
/// nsImage extends nsImageView by owning the image data it references. It can use either internal storage
/// or attach to external memory. This class handles allocation, deallocation, and provides convenient
/// methods for loading, saving, and converting images.
///
/// Memory management:
/// - Internal storage: nsImage allocates and manages its own memory
/// - External storage: nsImage references user-provided memory (user manages lifetime)
/// - Storage can be switched between internal and external as needed
///
/// The sub-images are stored in a predefined order compatible with DDS files:
/// For each array slice: mip level 0, mip level 1, ..., mip level N
/// For cubemaps: +X, -X, +Y, -Y, +Z, -Z faces in that order
/// For texture arrays: array slice 0, array slice 1, ..., array slice N
///
/// Common usage patterns:
/// ```cpp
/// // Load from file
/// nsImage image;
/// image.LoadFrom("texture.png");
///
/// // Create with specific format
/// nsImageHeader header;
/// header.SetImageFormat(nsImageFormat::R8G8B8A8_UNORM);
/// header.SetWidth(256); header.SetHeight(256);
/// nsImage image(header);
///
/// // Convert format
/// image.Convert(nsImageFormat::BC1_UNORM);
/// ```
class NS_TEXTURE_DLL nsImage : public nsImageView
{
  /// Use Reset() instead
  void operator=(const nsImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const nsImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit nsImage(const nsImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit nsImage(const nsImageHeader& header, nsByteBlobPtr externalData);

  /// \brief Constructor from image view (copies the image data to internal storage)
  explicit nsImage(const nsImageView& other);

public:
  NS_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Constructs an empty image.
  nsImage();

  /// \brief Move constructor
  nsImage(nsImage&& other);

  void operator=(nsImage&& rhs);

  /// \brief Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// \brief Allocates storage for an image with the given header.
  ///
  /// If currently using external storage and it's large enough, that storage will be reused.
  /// Otherwise, the image will detach from external storage and allocate internal storage.
  /// Any existing data is discarded.
  void ResetAndAlloc(const nsImageHeader& header);

  /// \brief Attaches the image to external storage provided by the user.
  ///
  /// The external storage must remain valid for the lifetime of this nsImage.
  /// The storage must be large enough to hold the image data described by the header.
  /// Use this when you want to avoid memory allocation or work with memory-mapped files.
  void ResetAndUseExternalStorage(const nsImageHeader& header, nsByteBlobPtr externalData);

  /// \brief Takes ownership of another image's data via move semantics.
  ///
  /// The other image is left in an empty state. If the other image uses external storage,
  /// this image will also reference that storage and inherit the lifetime requirements.
  void ResetAndMove(nsImage&& other);

  /// \brief Copies data from an image view into internal storage.
  ///
  /// If currently attached to external storage, the attachment is discarded and internal
  /// storage is allocated. The source view's data is copied completely.
  void ResetAndCopy(const nsImageView& other);

  /// \brief Convenience function to load the image from the given file.
  nsResult LoadFrom(nsStringView sFileName);

  /// \brief Convenience function to convert the image to the given format.
  nsResult Convert(nsImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  nsBlobPtr<T> GetBlobPtr();

  nsByteBlobPtr GetByteBlobPtr();

  using nsImageView::GetBlobPtr;
  using nsImageView::GetByteBlobPtr;

  /// \brief Returns a view to the given sub-image.
  nsImage GetSubImageView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0);

  using nsImageView::GetSubImageView;

  /// \brief Returns a view to a sub-plane.
  nsImage GetPlaneView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 uiPlaneIndex = 0);

  using nsImageView::GetPlaneView;

  /// \brief Returns a view to z slice of the image.
  nsImage GetSliceView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0);

  using nsImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  nsImage GetRowView(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 y = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0);

  using nsImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(nsUInt32 uiMipLevel = 0, nsUInt32 uiFace = 0, nsUInt32 uiArrayIndex = 0, nsUInt32 x = 0, nsUInt32 y = 0, nsUInt32 z = 0, nsUInt32 uiPlaneIndex = 0);

  using nsImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  nsBlob m_InternalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
