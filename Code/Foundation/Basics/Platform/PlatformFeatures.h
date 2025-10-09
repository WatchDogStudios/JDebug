#pragma once

// include the platform specific header
#include <Features_Platform.h>

#ifdef BUILDSYSTEM_ENABLE_GLFW_SUPPORT
#  define NS_SUPPORTS_GLFW NS_ON
#else
#  define NS_SUPPORTS_GLFW NS_OFF
#endif

// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef NS_SUPPORTS_FILE_ITERATORS
#  error "NS_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef NS_USE_POSIX_FILE_API
#  error "NS_USE_POSIX_FILE_API is not defined."
#endif

#ifndef NS_SUPPORTS_FILE_STATS
#  error "NS_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef NS_SUPPORTS_MEMORY_MAPPED_FILE
#  error "NS_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef NS_SUPPORTS_SHARED_MEMORY
#  error "NS_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef NS_SUPPORTS_DYNAMIC_PLUGINS
#  error "NS_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef NS_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "NS_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef NS_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "NS_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef NS_SUPPORTS_LONG_PATHS
#  error "NS_SUPPORTS_LONG_PATHS is not defined."
#endif

#ifndef NS_SUPPORTS_IPC
#  error "NS_SUPPORTS_IPC is not defined."
#endif

#if NS_IS_NOT_EXCLUSIVE(NS_PLATFORM_32BIT, NS_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#if NS_IS_NOT_EXCLUSIVE(NS_PLATFORM_LITTLE_ENDIAN, NS_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef NS_MATH_CHECK_FOR_NAN
#  error "NS_MATH_CHECK_FOR_NAN is not defined."
#endif

#if NS_IS_NOT_EXCLUSIVE3(NS_PLATFORM_ARCH_X86, NS_PLATFORM_ARCH_ARM, NS_PLATFORM_ARCH_WEB)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(NS_SIMD_IMPLEMENTATION) || (NS_SIMD_IMPLEMENTATION == 0)
#  error "NS_SIMD_IMPLEMENTATION is not correctly defined."
#endif

#ifndef NS_PLATFORM_NAME
#  error "NS_PLATFORM_NAME is not defined."
#endif
