#include <Foundation/Platform/PlatformDesc.h>

NS_ENUMERABLE_CLASS_IMPLEMENTATION(nsPlatformDesc);

nsPlatformDesc g_PlatformDescWin("Windows", "Desktop");

#if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)

const nsPlatformDesc* nsPlatformDesc::s_pThisPlatform = &g_PlatformDescWin;

#endif
