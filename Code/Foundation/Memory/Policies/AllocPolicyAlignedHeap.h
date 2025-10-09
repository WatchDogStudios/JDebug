#pragma once

#include <Foundation/Basics.h>

/// \brief Aligned Heap memory allocation policy.
///
/// \see nsAllocatorWithPolicy
class nsAllocPolicyAlignedHeap
{
public:
  NS_ALWAYS_INLINE nsAllocPolicyAlignedHeap(nsAllocator* pParent) { NS_IGNORE_UNUSED(pParent); }
  NS_ALWAYS_INLINE ~nsAllocPolicyAlignedHeap() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  NS_ALWAYS_INLINE nsAllocator* GetParent() const { return nullptr; }
};

// include the platform specific implementation
#include <AllocPolicyAlignedHeap_Platform.h>
