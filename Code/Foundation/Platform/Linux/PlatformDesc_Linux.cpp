#include <Foundation/Platform/PlatformDesc.h>

nsPlatformDesc g_PlatformDescLinux("Linux", "Desktop");

#if NS_ENABLED(NS_PLATFORM_LINUX)

const nsPlatformDesc* nsPlatformDesc::s_pThisPlatform = &g_PlatformDescLinux;

#endif
