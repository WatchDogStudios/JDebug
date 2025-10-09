#pragma once

#define NS_INCLUDING_BASICS_H

// Very basic Preprocessor defines
#include <Foundation/Basics/PreprocessorUtils.h>

// Set all feature #defines to NS_OFF
#include <Foundation/Basics/AllDefinesOff.h>

// General detection of the OS and hardware
#include <Foundation/Basics/Platform/DetectArchitecture.h>

// Here all the different features that each platform supports are declared.
#include <Foundation/Basics/Platform/PlatformFeatures.h>

// Options by the user to override the build
#include <Foundation/UserConfig.h>

// Configure the DLL Import/Export Define
#if NS_ENABLED(NS_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#    define NS_FOUNDATION_DLL NS_DECL_EXPORT
#    define NS_FOUNDATION_DLL_FRIEND NS_DECL_EXPORT_FRIEND
#  else
#    define NS_FOUNDATION_DLL NS_DECL_IMPORT
#    define NS_FOUNDATION_DLL_FRIEND NS_DECL_IMPORT_FRIEND
#  endif
#else
#  define NS_FOUNDATION_DLL
#  define NS_FOUNDATION_DLL_FRIEND
#endif

#include <Foundation/FoundationInternal.h>

// include headers for the supported compilers
#include <Foundation/Basics/Compiler/Clang.h>
#include <Foundation/Basics/Compiler/GCC.h>
#include <Foundation/Basics/Compiler/MSVC.h>

// Include common definitions and macros (e.g. static_assert)
#include <Foundation/Basics/Platform/Common.h>

// Include magic preprocessor macros
#include <Foundation/Basics/Platform/BlackMagic.h>

// Now declare all fundamental types
#include <Foundation/Types/Types.h>

// Type trait utilities
#include <Foundation/Types/TypeTraits.h>

// Assert macros should always be available
#include <Foundation/Basics/Assert.h>

// String formatting is needed by the asserts
#include <Foundation/Strings/FormatString.h>


class NS_FOUNDATION_DLL nsFoundation
{
public:
  static nsAllocator* s_pDefaultAllocator;
  static nsAllocator* s_pAlignedAllocator;

  /// \brief The default allocator can be used for any kind of allocation if no alignment is required
  NS_ALWAYS_INLINE static nsAllocator* GetDefaultAllocator()
  {
    if (s_bIsInitialized)
      return s_pDefaultAllocator;
    else // the default allocator is not yet set so we return the static allocator instead.
      return GetStaticsAllocator();
  }

  /// \brief The aligned allocator should be used for all allocations which need alignment
  NS_ALWAYS_INLINE static nsAllocator* GetAlignedAllocator()
  {
    NS_ASSERT_ALWAYS(s_pAlignedAllocator != nullptr,
      "nsFoundation must have been initialized before this function can be called."
      "This error can occur when you have a global variable or a static member variable that (indirectly) requires an allocator."
      "Check out the documentation for 'nsStaticsAllocatorWrapper' for more information about this issue.");
    return s_pAlignedAllocator;
  }

  /// \brief Returns the allocator that is used by global data and static members before the default allocator is created.
  static nsAllocator* GetStaticsAllocator();

private:
  friend class nsStartup;
  friend struct nsStaticsAllocatorWrapper;

  static void Initialize();
  static bool s_bIsInitialized;
};

#undef NS_INCLUDING_BASICS_H
