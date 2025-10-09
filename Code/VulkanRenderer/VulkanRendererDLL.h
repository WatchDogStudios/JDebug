#pragma once

#include <Foundation/Basics.h>

#if NS_ENABLED(NS_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_VULKANRENDERER_LIB
#    define NS_VULKANRENDERER_DLL NS_DECL_EXPORT
#  else
#    define NS_VULKANRENDERER_DLL NS_DECL_IMPORT
#  endif
#else
#  define NS_VULKANRENDERER_DLL
#endif
