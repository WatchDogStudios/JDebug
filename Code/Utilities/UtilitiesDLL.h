#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if NS_ENABLED(NS_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_UTILITIES_LIB
#    define NS_UTILITIES_DLL NS_DECL_EXPORT
#  else
#    define NS_UTILITIES_DLL NS_DECL_IMPORT
#  endif
#else
#  define NS_UTILITIES_DLL
#endif
