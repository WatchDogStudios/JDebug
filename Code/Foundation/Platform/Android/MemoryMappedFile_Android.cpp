#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)
#  define NS_POSIX_MMAP_SKIPUNLINK
#  include <Foundation/Platform/Posix/MemoryMappedFile_Posix.inl>
#endif
