#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
NS_WARNING_PUSH()
NS_WARNING_DISABLE_MSVC(4985)

// include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

NS_WARNING_POP()

// redefine NULL to nullptr
#ifdef NULL
#  undef NULL
#endif
#define NULL nullptr

// include c++11 specific header
#include <type_traits>
#include <utility>

/// \brief Disallow the copy constructor and the assignment operator for this type.
#define NS_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;             \
  void operator=(const type&) = delete

#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
/// \brief Macro helper to check alignment
#  define NS_CHECK_ALIGNMENT(ptr, alignment) NS_ASSERT_DEV(((size_t)ptr & ((alignment) - 1)) == 0, "Wrong alignment.")
#else
/// \brief Macro helper to check alignment
#  define NS_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define NS_WINCHECK_1 1          // NS_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define NS_WINCHECK_1_WINDOWS_ 1 // NS_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define NS_WINCHECK_NS_INCLUDED_WINDOWS_H \
  0                              // NS_INCLUDED_WINDOWS_H undefined (stringyfied to "NS_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define NS_WINCHECK_NS_INCLUDED_WINDOWS_H_WINDOWS_ \
  1                              // NS_INCLUDED_WINDOWS_H undefined (stringyfied to "NS_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// \brief Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define NS_CHECK_WINDOWS_INCLUDE(NS_WINH_INCLUDED, WINH_INCLUDED)                               \
  static_assert(NS_PP_CONCAT(NS_WINCHECK_, NS_PP_CONCAT(NS_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
    "Windows.h has been included but not through ns. #include <Foundation/Platform/Win/Utils/IncludeWindows.h> instead of Windows.h");

#if NS_ENABLED(NS_COMPILE_ENGINE_AS_DLL)

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of NS_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define NS_STATICLINK_FILE(LibraryName, UniqueName) NS_CHECK_WINDOWS_INCLUDE(NS_INCLUDED_WINDOWS_H, _WINDOWS_)

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after NS_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see NS_STATICLINK_FILE
#  define NS_STATICLINK_REFERENCE(UniqueName)

/// \brief This must occur exactly once in each static library, such that all NS_STATICLINK_FILE macros can reference it.
#  define NS_STATICLINK_LIBRARY(LibraryName) void nsReferenceFunction_##LibraryName(bool bReturn = true)

/// \brief Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// NS_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define NS_STATICLINK_PLUGIN(PluginName)

/// \brief A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example an nsEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define NS_STATICLINK_FORCE

#else

struct nsStaticLinkHelper
{
  using Func = void (*)(bool);
  nsStaticLinkHelper(Func f) { f(true); }
};

/// \brief Helper struct to register the existence of statically linked plugins.
/// The macro NS_STATICLINK_LIBRARY will register a the given library name prepended with `ns` to the nsPlugin system.
/// Implemented in Plugin.cpp.
struct NS_FOUNDATION_DLL nsPluginRegister
{
  nsPluginRegister(const char* szName);
};

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of NS_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define NS_STATICLINK_FILE(LibraryName, UniqueName)       \
    extern "C"                                              \
    {                                                       \
      void nsReferenceFunction_##UniqueName(bool bReturn)   \
      {                                                     \
        (void)bReturn;                                      \
      }                                                     \
      void nsReferenceFunction_##LibraryName(bool bReturn); \
    }                                                       \
    static nsStaticLinkHelper StaticLinkHelper_##UniqueName(nsReferenceFunction_##LibraryName);

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after NS_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see NS_STATICLINK_FILE
#  define NS_STATICLINK_REFERENCE(UniqueName)                   \
    void nsReferenceFunction_##UniqueName(bool bReturn = true); \
    nsReferenceFunction_##UniqueName()

/// \brief This must occur exactly once in each static library, such that all NS_STATICLINK_FILE macros can reference it.
#  define NS_STATICLINK_LIBRARY(LibraryName)                                                         \
    nsPluginRegister nsPluginRegister_##LibraryName(NS_PP_STRINGIFY(NS_PP_CONCAT(ns, LibraryName))); \
    extern "C" void nsReferenceFunction_##LibraryName(bool bReturn = true)

/// \brief Adds a static link reference to a plugin into an application, to make sure all code gets pulled in by the linker.
///
/// Add a line like this to a CPP file of your application:
/// NS_STATICLINK_PLUGIN(ParticlePlugin);
///
/// When statically linking, this ensures that all relevant code of that plugin gets added to your app.
/// Without it, the linker may optimize too much code away, such that, for example, component types are unknown at runtime.
///
/// When dynamic linking is used, this macro has no effect, at all.
#  define NS_STATICLINK_PLUGIN(PluginName)                                               \
    extern "C" void NS_PP_CONCAT(nsReferenceFunction_, PluginName)(bool bReturn = true); \
    nsStaticLinkHelper NS_PP_CONCAT(nsStaticLinkHelper_, PluginName)(NS_PP_CONCAT(nsReferenceFunction_, PluginName));

/// \brief A marker that can be placed in CPP files to enforce that the StaticLinkUtil doesn't skip this file.
///
/// Needed when a CPP file contains a global variable that's used for registering something (for example an nsEnumerable),
/// and there is no other indication for the StaticLinkUtil to consider the file.
#  define NS_STATICLINK_FORCE

#endif

namespace nsInternal
{
  template <typename T>
  constexpr bool AlwaysFalse = false;

  template <typename T>
  struct ArraySizeHelper
  {
    static_assert(AlwaysFalse<T>, "Cannot take compile time array size of given type");
  };

  template <typename T, size_t N>
  struct ArraySizeHelper<T[N]>
  {
    static constexpr size_t value = N;
  };

} // namespace nsInternal

/// \brief Macro to determine the size of a static array
#define NS_ARRAY_SIZE(a) (nsInternal::ArraySizeHelper<decltype(a)>::value)

/// \brief Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void NS_IGNORE_UNUSED(const T&)
{
}

#if (__cplusplus >= 202002L || _MSVC_LANG >= 202002L)
#  undef NS_USE_CPP20_OPERATORS
#  define NS_USE_CPP20_OPERATORS NS_ON
#endif

#if NS_ENABLED(NS_USE_CPP20_OPERATORS)
// in C++ 20 we don't need to declare an operator!=, it is automatically generated from operator==
#  define NS_ADD_DEFAULT_OPERATOR_NOTEQUAL(...) /*empty*/
#else
#  define NS_ADD_DEFAULT_OPERATOR_NOTEQUAL(...)                                   \
    NS_ALWAYS_INLINE bool operator!=(NS_EXPAND_ARGS_COMMA(__VA_ARGS__) rhs) const \
    {                                                                             \
      return !(*this == rhs);                                                     \
    }
#endif
