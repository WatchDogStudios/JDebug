#pragma once

#include <VulkanRenderer/VulkanRendererDLL.h>

#include <Foundation/Basics.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Unknwn.h>
#  include <Objidl.h>
#  include <OleAuto.h>
#endif

#include <dxcapi.h>

namespace nsVulkanDxc
{
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  using CreateInstanceProc = HRESULT(__stdcall*)(REFCLSID, REFIID, void**);
#else
  using CreateInstanceProc = HRESULT (*)(REFCLSID, REFIID, void**);
#endif

  /// \brief Resolves the DXC runtime entry point across platforms.
  ///
  /// On success, \a outProc receives a valid function pointer that can be used to create DXC interfaces.
  /// The function attempts to locate libdxcompiler dynamically on non-Windows platforms and caches the result.
  NS_VULKANRENDERER_DLL nsResult ResolveCreateInstance(CreateInstanceProc& outProc);
}
