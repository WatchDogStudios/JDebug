#pragma once

#include <Foundation/Basics.h>

/// \brief Default heap memory allocation policy.
///
/// \see nsAllocatorWithPolicy
class nsAllocPolicyHeap
{
public:
  NS_ALWAYS_INLINE nsAllocPolicyHeap(nsAllocator* pParent) { NS_IGNORE_UNUSED(pParent); }
  NS_ALWAYS_INLINE ~nsAllocPolicyHeap() = default;

  NS_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    NS_IGNORE_UNUSED(uiAlign);

    // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
    // if these asserts fail, you need to check what container made the allocation and change it
    // to use an aligned allocator, e.g. nsAlignedAllocatorWrapper

    // unfortunately using NS_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
    // alignment interestingly, the code that does that, seems to work fine anyway
    NS_ASSERT_DEBUG(uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

    void* ptr = malloc(uiSize);
    NS_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  NS_FORCE_INLINE void* Reallocate(void* pCurrentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    NS_IGNORE_UNUSED(uiCurrentSize);
    NS_IGNORE_UNUSED(uiAlign);

    void* ptr = realloc(pCurrentPtr, uiNewSize);
    NS_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  NS_ALWAYS_INLINE void Deallocate(void* pPtr)
  {
    free(pPtr);
  }

  NS_ALWAYS_INLINE nsAllocator* GetParent() const { return nullptr; }
};
