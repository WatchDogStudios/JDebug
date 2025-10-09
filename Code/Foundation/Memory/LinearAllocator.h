#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyLinear.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Stack-based linear allocator that allocates memory sequentially.
///
/// This allocator allocates memory by simply advancing a pointer within a pre-allocated buffer.
/// Individual deallocations are not supported - the entire allocator must be reset at once.
/// Very efficient for temporary allocations that follow a stack-like pattern.
///
/// Template parameters:
/// - TrackingMode: Controls allocation tracking and debugging features
/// - OverwriteMemoryOnReset: If true, fills memory with debug pattern on reset (debug builds only)
///
/// Performance characteristics:
/// - Allocation: O(1) - just pointer arithmetic
/// - Deallocation: Not supported individually
/// - Reset: O(1) - just resets pointer
/// - Memory overhead: Minimal
///
/// Use when:
/// - Temporary allocations with predictable lifetime
/// - Stack-like allocation pattern
/// - High allocation frequency (parsing, temporary buffers)
/// - Frame-based or scope-based memory management
template <nsAllocatorTrackingMode TrackingMode = nsAllocatorTrackingMode::Default, bool OverwriteMemoryOnReset = false>
class nsLinearAllocator : public nsAllocatorWithPolicy<nsAllocPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>
{
  using PolicyStack = nsAllocPolicyLinear<OverwriteMemoryOnReset>;

public:
  nsLinearAllocator(nsStringView sName, nsAllocator* pParent);
  ~nsLinearAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, nsMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void Deallocate(void* pPtr) override;

  /// \brief Resets the allocator, freeing all memory and calling destructors.
  ///
  /// This resets the allocation pointer to the beginning and calls destructors for all
  /// objects that were allocated with destructor functions. After reset, all previously
  /// allocated pointers become invalid and must not be used.
  void Reset();

private:
  struct DestructData
  {
    NS_DECLARE_POD_TYPE();

    nsMemoryUtils::DestructorFunction m_Func;
    void* m_Ptr;
  };

  nsMutex m_Mutex;
  nsDynamicArray<DestructData> m_DestructData;
  nsHashTable<void*, nsUInt32> m_PtrToDestructDataIndexTable;
};

#include <Foundation/Memory/Implementation/LinearAllocator_inl.h>
