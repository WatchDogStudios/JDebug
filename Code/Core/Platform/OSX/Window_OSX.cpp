#include <Core/CorePCH.h>

#if NS_ENABLED(NS_PLATFORM_OSX)

#  if NS_DISABLED(NS_SUPPORTS_GLFW)
#    include <Core/Platform/NoImpl/Window_NoImpl.inl>
#  endif

#endif
