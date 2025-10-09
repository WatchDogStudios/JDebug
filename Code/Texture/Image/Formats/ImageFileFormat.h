#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>

class nsStreamReader;
class nsStreamWriter;
class nsImage;
class nsImageView;
class nsStringBuilder;
class nsImageHeader;

class NS_TEXTURE_DLL nsImageFileFormat
{
public:
  /// \brief Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual nsResult ReadImageHeader(nsStreamReader& inout_stream, nsImageHeader& ref_header, nsStringView sFileExtension) const = 0;

  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given nsLogInterface.
  virtual nsResult ReadImage(nsStreamReader& inout_stream, nsImage& ref_image, nsStringView sFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given nsLogInterface.
  virtual nsResult WriteImage(nsStreamWriter& inout_stream, const nsImageView& image, nsStringView sFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(nsStringView sExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(nsStringView sExtension) const = 0;

  /// \brief Returns an nsImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate nsImageFileFormat.
  static const nsImageFileFormat* GetReaderFormat(nsStringView sExtension);

  /// \brief Returns an nsImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate nsImageFileFormat.
  static const nsImageFileFormat* GetWriterFormat(nsStringView sExtension);

  static nsResult ReadImageHeader(nsStringView sFileName, nsImageHeader& ref_header);
};

/// \brief Base class for a registered (globally known) nsImageFileFormat.
///
/// This is an enumerable class, so all known formats can be retrieved through the nsEnumerable interface.
/// For example:
///
///   for (auto format = nsRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
///   {
///     auto& type = format->GetFormatType();
///   }
class NS_TEXTURE_DLL nsRegisteredImageFileFormat : public nsEnumerable<nsRegisteredImageFileFormat>
{
  NS_DECLARE_ENUMERABLE_CLASS(nsRegisteredImageFileFormat);

public:
  nsRegisteredImageFileFormat();
  ~nsRegisteredImageFileFormat();

  virtual const nsImageFileFormat& GetFormatType() const = 0;
};

/// \brief Template used to automatically register an nsImageFileFormat globally.
///
/// Place a global variable of the desired type in some CPP file to register the type:
///
///   nsImageFileFormatRegistrator<nsDdsFileFormat> g_ddsFormat;
///
/// For the format to be available on platforms that use static linking, you may also need to add
///   NS_STATICLINK_FORCE
template <class TYPE>
class nsImageFileFormatRegistrator : public nsRegisteredImageFileFormat
{
public:
  virtual const nsImageFileFormat& GetFormatType() const override
  {
    return m_Format;
  }

private:
  TYPE m_Format;
};
