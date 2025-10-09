#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringView.h>

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32
/// Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class NS_FOUNDATION_DLL nsStringWChar
{
public:
  nsStringWChar(nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringWChar(const nsUInt16* pUtf16, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringWChar(const nsUInt32* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringWChar(const wchar_t* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringWChar(nsStringView sUtf8, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());

  void operator=(const nsUInt16* pUtf16);
  void operator=(const nsUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);
  void operator=(nsStringView sUtf8);

  NS_ALWAYS_INLINE operator const wchar_t*() const { return &m_Data[0]; }
  NS_ALWAYS_INLINE const wchar_t* GetData() const { return &m_Data[0]; }
  NS_ALWAYS_INLINE nsUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr nsUInt32 BufferSize = 1024;
  nsHybridArray<wchar_t, BufferSize> m_Data;
};


/// \brief A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class NS_FOUNDATION_DLL nsStringUtf8
{
public:
  nsStringUtf8(nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf8(const char* szUtf8, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf8(const nsUInt16* pUtf16, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf8(const nsUInt32* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf8(const wchar_t* pWChar, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const nsUInt16* pUtf16);
  void operator=(const nsUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  NS_ALWAYS_INLINE operator const char*() const
  {
    return &m_Data[0];
  }
  NS_ALWAYS_INLINE const char* GetData() const
  {
    return &m_Data[0];
  }
  NS_ALWAYS_INLINE nsUInt32 GetElementCount() const
  {
    return m_Data.GetCount() - 1; /* exclude the '\0' terminator */
  }
  NS_ALWAYS_INLINE operator nsStringView() const
  {
    return GetView();
  }
  NS_ALWAYS_INLINE nsStringView GetView() const
  {
    return nsStringView(&m_Data[0], GetElementCount());
  }

private:
  static constexpr nsUInt32 BufferSize = 1024;
  nsHybridArray<char, BufferSize> m_Data;
};



/// \brief A very simple class to convert text to Utf16 encoding.
///
/// Use this class only temporarily, if you need to output something in Utf16 format, e.g. for writing it to a file.
/// Never use this for storage.
/// When working with OS functions that expect '16 Bit strings', use nsStringWChar instead.
class NS_FOUNDATION_DLL nsStringUtf16
{
public:
  nsStringUtf16(nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf16(const char* szUtf8, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf16(const nsUInt16* pUtf16, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf16(const nsUInt32* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf16(const wchar_t* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const nsUInt16* pUtf16);
  void operator=(const nsUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);

  NS_ALWAYS_INLINE const nsUInt16* GetData() const { return &m_Data[0]; }
  NS_ALWAYS_INLINE nsUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr nsUInt32 BufferSize = 1024;
  nsHybridArray<nsUInt16, BufferSize> m_Data;
};



/// \brief This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class NS_FOUNDATION_DLL nsStringUtf32
{
public:
  nsStringUtf32(nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf32(const char* szUtf8, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf32(const nsUInt16* pUtf16, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf32(const nsUInt32* pUtf32, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());
  nsStringUtf32(const wchar_t* pWChar, nsAllocator* pAllocator = nsFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const nsUInt16* pUtf16);
  void operator=(const nsUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  NS_ALWAYS_INLINE const nsUInt32* GetData() const { return &m_Data[0]; }
  NS_ALWAYS_INLINE nsUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr nsUInt32 BufferSize = 1024;
  nsHybridArray<nsUInt32, BufferSize> m_Data;
};

#include <Foundation/Strings/Implementation/StringConversion_inl.h>
