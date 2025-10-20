#pragma once

#include <DirectX11Renderer/DirectX11RendererDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <d3d11.h>
#  include <dxgi.h>
#  include <wrl/client.h>
#endif
