#pragma once

#include <Foundation/Basics.h>

#if NS_ENABLED(NS_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_PVDRENDERER_LIB
#    define NS_PVDRENDERER_DLL NS_DECL_EXPORT
#  else
#    define NS_PVDRENDERER_DLL NS_DECL_IMPORT
#  endif
#else
#  define NS_PVDRENDERER_DLL
#endif
