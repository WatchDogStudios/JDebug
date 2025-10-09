#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadUtils.h>

/// \brief Represents a typed block of memory with fixed size, typically used for bulk allocations.
///
/// This wrapper provides type-safe access to a block of memory that can hold multiple elements
/// of type T. The block has a fixed capacity determined by SizeInBytes and sizeof(T).
/// It tracks the current count of used elements and provides stack-like operations for
/// efficient allocation/deallocation within the block.
template <typename T, nsUInt32 SizeInBytes>
struct nsDataBlock
{
  NS_DECLARE_POD_TYPE();

  enum
  {
    SIZE_IN_BYTES = SizeInBytes,
    CAPACITY = SIZE_IN_BYTES / sizeof(T)
  };

  /// \brief Constructs a data block wrapping the given memory region.
  nsDataBlock(T* pData, nsUInt32 uiCount);

  /// \brief Reserves space for one element at the end of the block.
  ///
  /// Returns pointer to the reserved element, or nullptr if the block is full.
  T* ReserveBack();

  /// \brief Removes and returns pointer to the last element in the block.
  ///
  /// Returns nullptr if the block is empty.
  T* PopBack();

  bool IsEmpty() const;
  bool IsFull() const;

  /// \brief Provides access to elements by index within the used range.
  T& operator[](nsUInt32 uiIndex) const;

  T* m_pData;
  nsUInt32 m_uiCount;
};

/// \brief Specialized allocator for fixed-size memory blocks, optimized for bulk allocations.
///
/// This allocator manages memory in large chunks called "SuperBlocks" (16 blocks each) and
/// provides individual blocks of the specified size on demand. It's designed for scenarios
/// where you need many identically-sized allocations with good spatial locality.
///
/// SuperBlock strategy reduces fragmentation and improves cache performance by grouping
/// related allocations together. When blocks are freed, they're added to a free list for
/// immediate reuse without returning memory to the OS.
///
/// Best used for:
/// - Object pools where objects have uniform size
/// - Bulk allocations for data structures like arrays or strings
/// - Memory regions that benefit from spatial locality
template <nsUInt32 BlockSizeInByte>
class nsLargeBlockAllocator
{
public:
  nsLargeBlockAllocator(nsStringView sName, nsAllocator* pParent, nsAllocatorTrackingMode mode = nsAllocatorTrackingMode::Default);
  ~nsLargeBlockAllocator();

  /// \brief Allocates a new typed block capable of holding elements of type T.
  ///
  /// Returns a typed wrapper around a raw memory block. The block can hold
  /// BlockSizeInByte / sizeof(T) elements. If allocation fails, returns an
  /// invalid block (check with IsEmpty()).
  template <typename T>
  nsDataBlock<T, BlockSizeInByte> AllocateBlock();

  /// \brief Deallocates a previously allocated block.
  template <typename T>
  void DeallocateBlock(nsDataBlock<T, BlockSizeInByte>& ref_block);


  nsStringView GetName() const;

  /// \brief Returns the unique identifier for this allocator instance.
  nsAllocatorId GetId() const;

  const nsAllocator::Stats& GetStats() const;

private:
  void* Allocate(size_t uiAlign);
  void Deallocate(void* ptr);

  nsAllocatorId m_Id;
  nsAllocatorTrackingMode m_TrackingMode;

  nsMutex m_Mutex;

  struct SuperBlock
  {
    NS_DECLARE_POD_TYPE();

    enum
    {
      NUM_BLOCKS = 16,
      SIZE_IN_BYTES = BlockSizeInByte * NUM_BLOCKS
    };

    void* m_pBasePtr;

    nsUInt32 m_uiUsedBlocks;
  };

  nsDynamicArray<SuperBlock> m_SuperBlocks;
  nsDynamicArray<nsUInt32> m_FreeBlocks;
};

#include <Foundation/Memory/Implementation/LargeBlockAllocator_inl.h>
