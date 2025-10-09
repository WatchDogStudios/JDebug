#pragma once

#include <Foundation/Memory/Allocator.h>

/// \brief Allocator wrapper that should never be used - causes assertion failures.
///
/// This wrapper is used as a template parameter to indicate that no allocator
/// should be used. Any attempt to call GetAllocator() will trigger an assertion.
/// Useful for container types that should never allocate.
struct nsNullAllocatorWrapper
{
  NS_FORCE_INLINE static nsAllocator* GetAllocator()
  {
    NS_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

/// \brief Wrapper for the engine's default general-purpose allocator.
struct nsDefaultAllocatorWrapper
{
  NS_ALWAYS_INLINE static nsAllocator* GetAllocator() { return nsFoundation::GetDefaultAllocator(); }
};

/// \brief Wrapper for the allocator used for static/global objects.
struct nsStaticsAllocatorWrapper
{
  NS_ALWAYS_INLINE static nsAllocator* GetAllocator() { return nsFoundation::GetStaticsAllocator(); }
};

/// \brief Wrapper for the allocator that provides memory with specific alignment guarantees.
struct nsAlignedAllocatorWrapper
{
  NS_ALWAYS_INLINE static nsAllocator* GetAllocator() { return nsFoundation::GetAlignedAllocator(); }
};

/// \brief Helper function to facilitate setting the allocator on member containers of a class
/// Allocators can be either template arguments or a ctor parameter. Using the ctor parameter requires the class ctor to reference each member container in the initialization list. This can be very tedious. On the other hand, the template variant only support template parameter so you can't simply pass in a member allocator.
/// This class solves this problem provided the following rules are followed:
/// 1. The `nsAllocator` must be the declared at the earliest in the class, before any container.
/// 2. The `nsLocalAllocatorWrapper` should be declared right afterwards.
/// 3. Any container needs to be declared below these two and must include the `nsLocalAllocatorWrapper` as a template argument to the allocator.
/// 4. In the ctor initializer list, init the nsAllocator first, then the nsLocalAllocatorWrapper. With this approach all containers can be omitted.
/// \code{.cpp}
///   class MyClass
///   {
///     nsAllocator m_SpecialAlloc;
///     nsLocalAllocatorWrapper m_Wrapper;
///
///     nsDynamicArray<int, nsLocalAllocatorWrapper> m_Data;
///
///     MyClass()
///       : m_SpecialAlloc("MySpecialAlloc")
///       , m_Wrapper(&m_SpecialAlloc)
///     {
///     }
///   }
/// \endcode
struct NS_FOUNDATION_DLL nsLocalAllocatorWrapper
{
  nsLocalAllocatorWrapper(nsAllocator* pAllocator);

  void Reset();

  static nsAllocator* GetAllocator();
};
