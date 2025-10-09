
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

using nsTypeVersion = nsUInt16;

template <nsUInt16 Size, typename AllocatorWrapper>
struct nsHybridString;

using nsString = nsHybridString<32, nsDefaultAllocatorWrapper>;

/// \brief Abstract base class for binary input streams providing unified reading interface.
///
/// StreamReader defines the fundamental interface for reading binary data from various sources
/// including files, memory buffers, network connections, and compressed streams. All read
/// operations are performed in a sequential manner with automatic endianness handling.
///
/// Core functionality:
/// - ReadBytes(): Raw binary data reading (pure virtual, must be implemented)
/// - Typed reading methods with endianness conversion (ReadWordValue, ReadDWordValue, etc.)
/// - High-level container reading (arrays, sets, maps, hash tables)
/// - String reading with automatic length handling
///
/// Implementation requirements:
/// - Derived classes must implement ReadBytes() as the primary read method
/// - All other methods are implemented in terms of ReadBytes()
/// - Should handle EOF conditions gracefully by returning actual bytes read
class NS_FOUNDATION_DLL nsStreamReader
{
  NS_DISALLOW_COPY_AND_ASSIGN(nsStreamReader);

public:
  /// \brief Constructor
  nsStreamReader();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~nsStreamReader();

  /// \brief Reads a raw number of bytes into the read buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pReadBuffer Destination buffer for the read data
  /// \param uiBytesToRead Maximum number of bytes to read
  /// \return Actual number of bytes read (may be less than requested on EOF or error)
  virtual nsUInt64 ReadBytes(void* pReadBuffer, nsUInt64 uiBytesToRead) = 0; // [tested]

  /// \brief Helper method to read a word value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult ReadWordValue(T* pWordValue); // [tested]

  /// \brief Helper method to read a dword value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult ReadDWordValue(T* pDWordValue); // [tested]

  /// \brief Helper method to read a qword value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult ReadQWordValue(T* pQWordValue); // [tested]

  /// \brief Reads an array of elements from the stream
  template <typename ArrayType, typename ValueType>
  nsResult ReadArray(nsArrayBase<ValueType, ArrayType>& inout_array); // [tested]

  /// \brief Reads a small array of elements from the stream
  template <typename ValueType, nsUInt16 uiSize, typename AllocatorWrapper>
  nsResult ReadArray(nsSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, nsUInt32 uiSize>
  nsResult ReadArray(ValueType (&array)[uiSize]);

  /// \brief Reads a set
  template <typename KeyType, typename Comparer>
  nsResult ReadSet(nsSetBase<KeyType, Comparer>& inout_set); // [tested]

  /// \brief Reads a map
  template <typename KeyType, typename ValueType, typename Comparer>
  nsResult ReadMap(nsMapBase<KeyType, ValueType, Comparer>& inout_map); // [tested]

  /// \brief Read a hash table (note that the entry order is not stable)
  template <typename KeyType, typename ValueType, typename Hasher>
  nsResult ReadHashTable(nsHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable); // [tested]

  /// \brief Reads a string into an nsStringBuilder
  nsResult ReadString(nsStringBuilder& ref_sBuilder); // [tested]

  /// \brief Reads a string into an nsString
  nsResult ReadString(nsString& ref_sString);


  /// \brief Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example)
  virtual nsUInt64 SkipBytes(nsUInt64 uiBytesToSkip)
  {
    nsUInt8 uiTempBuffer[1024];

    nsUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytesToSkip)
    {
      nsUInt64 uiBytesToRead = nsMath::Min<nsUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      nsUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }

  NS_ALWAYS_INLINE nsTypeVersion ReadVersion(nsTypeVersion expectedMaxVersion);
};

/// \brief Abstract base class for binary output streams providing unified writing interface.
///
/// StreamWriter defines the fundamental interface for writing binary data to various destinations
/// including files, memory buffers, network connections, and compressed streams. All write
/// operations are performed sequentially with automatic endianness handling.
///
/// Core functionality:
/// - WriteBytes(): Raw binary data writing (pure virtual, must be implemented)
/// - Typed writing methods with endianness conversion (WriteWordValue, WriteDWordValue, etc.)
/// - High-level container writing (arrays, sets, maps, hash tables)
/// - String writing with automatic length prefixing
/// - Optional flush support for ensuring data persistence
///
/// Implementation requirements:
/// - Derived classes must implement WriteBytes() as the primary write method
/// - All other methods are implemented in terms of WriteBytes()
/// - Flush() can be overridden for buffered implementations
/// - Should handle write errors gracefully by returning appropriate nsResult
class NS_FOUNDATION_DLL nsStreamWriter
{
  NS_DISALLOW_COPY_AND_ASSIGN(nsStreamWriter);

public:
  /// \brief Constructor
  nsStreamWriter();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~nsStreamWriter();

  /// \brief Writes a raw number of bytes from the buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pWriteBuffer Source buffer containing data to write
  /// \param uiBytesToWrite Number of bytes to write from the buffer
  /// \return NS_SUCCESS if all bytes were written successfully, NS_FAILURE otherwise
  virtual nsResult WriteBytes(const void* pWriteBuffer, nsUInt64 uiBytesToWrite) = 0; // [tested]

  /// \brief Flushes buffered data to the underlying storage, ensuring data persistence.
  ///
  /// Default implementation is a no-op. Derived classes with internal buffering should
  /// override this method to force writing of buffered data to the actual destination.
  virtual nsResult Flush() // [tested]
  {
    return NS_SUCCESS;
  }

  /// \brief Helper method to write a word value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult WriteWordValue(const T* pWordValue); // [tested]

  /// \brief Helper method to write a dword value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult WriteDWordValue(const T* pDWordValue); // [tested]

  /// \brief Helper method to write a qword value correctly (copes with potentially different endianess)
  template <typename T>
  nsResult WriteQWordValue(const T* pQWordValue); // [tested]

  /// \brief Writes a type version to the stream
  NS_ALWAYS_INLINE void WriteVersion(nsTypeVersion version);

  /// \brief Writes an array of elements to the stream
  template <typename ArrayType, typename ValueType>
  nsResult WriteArray(const nsArrayBase<ValueType, ArrayType>& array); // [tested]

  /// \brief Writes a small array of elements to the stream
  template <typename ValueType, nsUInt16 uiSize>
  nsResult WriteArray(const nsSmallArrayBase<ValueType, uiSize>& array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, nsUInt32 uiSize>
  nsResult WriteArray(const ValueType (&array)[uiSize]);

  /// \brief Writes a set
  template <typename KeyType, typename Comparer>
  nsResult WriteSet(const nsSetBase<KeyType, Comparer>& set); // [tested]

  /// \brief Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  nsResult WriteMap(const nsMapBase<KeyType, ValueType, Comparer>& map); // [tested]

  /// \brief Writes a hash table (note that the entry order might change on read)
  template <typename KeyType, typename ValueType, typename Hasher>
  nsResult WriteHashTable(const nsHashTableBase<KeyType, ValueType, Hasher>& hashTable); // [tested]

  /// \brief Writes a string
  nsResult WriteString(const nsStringView sStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>
