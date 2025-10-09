#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

/// \brief Defines storage strategies for block-based container management.
struct nsBlockStorageType
{
  enum Enum
  {
    Compact, ///< Maintains elements in contiguous memory by moving last element to fill gaps
    FreeList ///< Uses a free list to track available slots, preserving element positions
  };
};

/// \brief High-performance container for objects with pluggable storage strategies.
///
/// This container manages objects in blocks of memory, using different strategies for handling
/// gaps when objects are removed. It's designed for scenarios where you need fast allocation
/// and deallocation of many objects, with the choice between compact memory layout or stable
/// object addressing.
///
/// Storage strategies:
/// - Compact: Moves the last element to fill gaps when objects are deleted, maintaining
///   contiguous memory but invalidating iterators and pointers to moved objects
/// - FreeList: Uses a free list to reuse deleted slots, preserving object positions but
///   potentially creating memory fragmentation
template <typename T, nsUInt32 BlockSizeInByte, nsBlockStorageType::Enum StorageType>
class nsBlockStorage
{
public:
  class ConstIterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();
    bool IsValid() const;

    void operator++();

  protected:
    friend class nsBlockStorage<T, BlockSizeInByte, StorageType>;

    ConstIterator(const nsBlockStorage<T, BlockSizeInByte, StorageType>& storage, nsUInt32 uiStartIndex, nsUInt32 uiCount);

    T& CurrentElement() const;

    const nsBlockStorage<T, BlockSizeInByte, StorageType>& m_Storage;
    nsUInt32 m_uiCurrentIndex;
    nsUInt32 m_uiEndIndex;
  };

  class Iterator : public ConstIterator
  {
  public:
    T& operator*();
    T* operator->();

    operator T*();

  private:
    friend class nsBlockStorage<T, BlockSizeInByte, StorageType>;

    Iterator(const nsBlockStorage<T, BlockSizeInByte, StorageType>& storage, nsUInt32 uiStartIndex, nsUInt32 uiCount);
  };

  nsBlockStorage(nsLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, nsAllocator* pAllocator);
  ~nsBlockStorage();

  /// \brief Removes all objects and deallocates all blocks.
  void Clear();

  /// \brief Creates a new object and returns a pointer to it.
  ///
  /// The object is default-constructed. Returns nullptr if allocation fails.
  T* Create();

  /// \brief Deletes the specified object.
  void Delete(T* pObject);

  /// \brief Deletes the specified object and reports any moved object.
  ///
  /// For Compact storage, if another object is moved to fill the gap,
  /// out_pMovedObject will point to the moved object's new location.
  /// For FreeList storage, out_pMovedObject is always set to nullptr.
  void Delete(T* pObject, T*& out_pMovedObject);

  /// \brief Returns the total number of objects currently stored.
  nsUInt32 GetCount() const;

  /// \brief Returns an iterator for traversing objects in a specified range.
  Iterator GetIterator(nsUInt32 uiStartIndex = 0, nsUInt32 uiCount = nsInvalidIndex);

  /// \brief Returns a const iterator for traversing objects in a specified range.
  ConstIterator GetIterator(nsUInt32 uiStartIndex = 0, nsUInt32 uiCount = nsInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, nsTraitInt<nsBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, nsTraitInt<nsBlockStorageType::FreeList>);

  nsLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  nsDynamicArray<nsDataBlock<T, BlockSizeInByte>> m_Blocks;
  nsUInt32 m_uiCount = 0;

  nsUInt32 m_uiFreelistStart = nsInvalidIndex;

  nsDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
