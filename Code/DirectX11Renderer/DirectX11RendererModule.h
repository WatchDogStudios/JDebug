#pragma once

#include <DirectX11Renderer/DirectX11RendererDLL.h>

#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <d3d11.h>
#  include <wrl/client.h>

using Microsoft::WRL::ComPtr;
#endif

struct nsDirectX11RendererCreateInfo
{
  void* m_pWindowHandle = nullptr;
  nsUInt32 m_uiWidth = 0;
  nsUInt32 m_uiHeight = 0;
  bool m_bEnableDebugLayer = true;
};

struct nsDirectX11InstanceData
{
  nsMat4 m_ModelMatrix = nsMat4::MakeIdentity();
  nsColor m_Color = nsColor::White;
  bool m_bSleeping = false;
};

class NS_DIRECTX11RENDERER_DLL nsDirectX11Renderer
{
public:
  nsDirectX11Renderer();
  ~nsDirectX11Renderer();

  nsResult Initialize(const nsDirectX11RendererCreateInfo& createInfo);
  void Deinitialize();
  nsResult RenderFrame();
  void SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight);
  void UpdateScene(const nsMat4& mViewProjection, nsArrayPtr<const nsDirectX11InstanceData> instances);
  bool IsInitialized() const;

private:
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  struct ConstantBufferData
  {
    nsMat4 m_ViewProjection = nsMat4::MakeIdentity();
  };

  struct PushConstantData
  {
    nsMat4 m_Model = nsMat4::MakeIdentity();
    nsColor m_Color = nsColor::White;
  };

  nsResult CreateDevice();
  nsResult CreateSwapChain(HWND hWnd, nsUInt32 uiWidth, nsUInt32 uiHeight);
  nsResult RecreateSwapChain();
  nsResult CreateRenderTargetView();
  nsResult CreateDepthStencilView();
  nsResult CreateShaders();
  nsResult CreateInputLayout();
  nsResult CreateConstantBuffers();
  nsResult CreateGeometryBuffers();
  nsResult CreateRasterizerState();
  nsResult CreateBlendState();
  nsResult CreateDepthStencilState();
  void CleanupSwapChainResources();
  void CleanupDeviceResources();

  ComPtr<ID3D11Device> m_pDevice;
  ComPtr<ID3D11DeviceContext> m_pContext;
  ComPtr<IDXGISwapChain> m_pSwapChain;
  ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
  ComPtr<ID3D11Texture2D> m_pDepthStencilBuffer;
  ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;
  ComPtr<ID3D11InputLayout> m_pInputLayout;
  ComPtr<ID3D11Buffer> m_pConstantBuffer;
  ComPtr<ID3D11Buffer> m_pInstanceConstantBuffer;
  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11RasterizerState> m_pRasterizerState;
  ComPtr<ID3D11BlendState> m_pBlendState;
  ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;

  HWND m_hWnd = nullptr;
  nsUInt32 m_uiWidth = 0;
  nsUInt32 m_uiHeight = 0;
  nsUInt32 m_uiIndexCount = 0;
  bool m_bEnableDebugLayer = true;
  bool m_bResizePending = false;

  nsMat4 m_viewProjection = nsMat4::MakeIdentity();
  nsDynamicArray<nsDirectX11InstanceData> m_sceneInstances;
#endif
};
