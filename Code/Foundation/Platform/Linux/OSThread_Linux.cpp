#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_LINUX)
#  define NS_POSIX_THREAD_SETNAME
#  include <Foundation/Platform/Posix/OSThread_Posix.inl>
#endif
