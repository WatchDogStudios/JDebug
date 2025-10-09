
#pragma once

#include <Foundation/Basics.h>

/// \file
///
/// Scope exit utilities for RAII-style cleanup operations.

/// \brief Executes code automatically when the current scope closes
///
/// Provides a convenient way to ensure cleanup code runs when leaving a scope,
/// regardless of how the scope is exited (normal return, exception, early return).
/// The code is executed in a destructor, guaranteeing cleanup even during stack unwinding.
///
/// Example usage:
/// ```cpp
/// {
///   FILE* file = fopen("test.txt", "r");
///   NS_SCOPE_EXIT(if (file) fclose(file););
///   // file will be closed automatically when scope ends
/// }
/// ```
#define NS_SCOPE_EXIT(code) auto NS_PP_CONCAT(scopeExit_, NS_SOURCE_LINE) = nsMakeScopeExit([&]() { code; })

/// \internal Helper class implementing RAII scope exit functionality
///
/// Stores a callable object and executes it in the destructor. Used internally
/// by the NS_SCOPE_EXIT macro to provide exception-safe cleanup operations.
template <typename T>
struct nsScopeExit
{
  NS_ALWAYS_INLINE nsScopeExit(T&& func)
    : m_func(std::forward<T>(func))
  {
  }

  NS_ALWAYS_INLINE ~nsScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement NS_SCOPE_EXIT
template <typename T>
NS_ALWAYS_INLINE nsScopeExit<T> nsMakeScopeExit(T&& func)
{
  return nsScopeExit<T>(std::forward<T>(func));
}
