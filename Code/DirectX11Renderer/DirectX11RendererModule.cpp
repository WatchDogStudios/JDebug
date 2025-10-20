#include <DirectX11Renderer/DirectX11RendererPCH.h>
#include <DirectX11Renderer/DirectX11RendererModule.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Containers/HybridArray.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <d3dcompiler.h>

#  pragma comment(lib, "d3d11.lib")
#  pragma comment(lib, "dxgi.lib")
#  pragma comment(lib, "d3dcompiler.lib")

namespace
{
  constexpr const char* s_szVertexShaderPath = ":base/Shaders/DirectX11Renderer/PvdSceneVS.hlsl";
  constexpr const char* s_szPixelShaderPath = ":base/Shaders/DirectX11Renderer/PvdScenePS.hlsl";

  nsResult LoadShaderSource(const char* szPath, nsDynamicArray<char>& outBuffer)
  {
    nsFileReader file;
    nsHybridArray<nsString, 8> attemptedPaths;
    nsString openedPath;

    auto TryOpen = [&](nsStringView path) -> bool {
      if (path.IsEmpty())
        return false;

      attemptedPaths.PushBack(path);
      if (file.Open(path).Succeeded())
      {
        openedPath = path;
        return true;
      }
      return false;
    };

    if (!TryOpen(szPath))
    {
      nsStringBuilder relativePath;

      // Handle :base/ prefix for data directory references
      if (nsStringUtils::IsEqualN(szPath, ":base/", 6))
      {
        relativePath = "Data/Base/";
        relativePath.Append(szPath + 6);
      }
      else if (szPath[0] == ':' && szPath[1] != '\0')
      {
        const char* szSlash = nsStringUtils::FindSubString(szPath, "/");
        if (szSlash != nullptr && *(szSlash + 1) != '\0')
        {
          relativePath = szSlash + 1;
        }
      }
      else
      {
        relativePath = szPath;
      }

      if (!relativePath.IsEmpty())
      {
        TryOpen(relativePath.GetView());
      }

      nsHybridArray<nsString, 4> searchRoots;
      nsStringBuilder root = nsOSFile::GetApplicationDirectory();
      root.MakeCleanPath();

      for (nsUInt32 i = 0; i < 4 && !root.IsEmpty(); ++i)
      {
        searchRoots.PushBack(root.GetView());

        const nsUInt32 uiLengthBefore = root.GetCharacterCount();
        root.PathParentDirectory();
        root.MakeCleanPath();

        if (root.IsEmpty() || root.GetCharacterCount() >= uiLengthBefore)
          break;
      }

      for (const nsString& rootPath : searchRoots)
      {
        nsStringBuilder candidate = rootPath;
        candidate.AppendPath(relativePath);
        candidate.MakeCleanPath();

        if (TryOpen(candidate.GetView()))
          break;
      }
    }

    if (openedPath.IsEmpty())
    {
      nsStringBuilder attemptsList;
      for (nsUInt32 i = 0; i < attemptedPaths.GetCount(); ++i)
      {
        if (i > 0)
          attemptsList.Append(", ");
        attemptsList.Append(attemptedPaths[i]);
      }

      nsLog::Error("Failed to open shader file '{0}'. Tried: {1}", szPath, attemptsList);
      return NS_FAILURE;
    }

    const nsUInt64 uiFileSize = file.GetFileSize();
    outBuffer.SetCount(static_cast<nsUInt32>(uiFileSize + 1));

    const nsUInt64 uiBytesRead = file.ReadBytes(outBuffer.GetData(), uiFileSize);
    if (uiBytesRead != uiFileSize)
    {
      nsLog::Error("Failed to read shader file '{0}'", openedPath);
      return NS_FAILURE;
    }

    outBuffer[static_cast<nsUInt32>(uiFileSize)] = '\0';
    return NS_SUCCESS;
  }
}

nsDirectX11Renderer::nsDirectX11Renderer() = default;

nsDirectX11Renderer::~nsDirectX11Renderer()
{
  Deinitialize();
}

nsResult nsDirectX11Renderer::Initialize(const nsDirectX11RendererCreateInfo& createInfo)
{
  NS_LOG_BLOCK("DirectX11Renderer::Initialize");

  if (!createInfo.m_pWindowHandle)
  {
    nsLog::Error("Window handle is null");
    return NS_FAILURE;
  }

  m_hWnd = static_cast<HWND>(createInfo.m_pWindowHandle);
  m_uiWidth = nsMath::Max<nsUInt32>(1, createInfo.m_uiWidth);
  m_uiHeight = nsMath::Max<nsUInt32>(1, createInfo.m_uiHeight);
  m_bEnableDebugLayer = createInfo.m_bEnableDebugLayer;

  NS_SUCCEED_OR_RETURN(CreateDevice());
  NS_SUCCEED_OR_RETURN(CreateSwapChain(m_hWnd, m_uiWidth, m_uiHeight));
  NS_SUCCEED_OR_RETURN(CreateRenderTargetView());
  NS_SUCCEED_OR_RETURN(CreateDepthStencilView());
  NS_SUCCEED_OR_RETURN(CreateShaders());
  NS_SUCCEED_OR_RETURN(CreateInputLayout());
  NS_SUCCEED_OR_RETURN(CreateConstantBuffers());
  NS_SUCCEED_OR_RETURN(CreateGeometryBuffers());
  NS_SUCCEED_OR_RETURN(CreateRasterizerState());
  NS_SUCCEED_OR_RETURN(CreateBlendState());
  NS_SUCCEED_OR_RETURN(CreateDepthStencilState());

  nsLog::Success("DirectX11 renderer initialized successfully");
  return NS_SUCCESS;
}

void nsDirectX11Renderer::Deinitialize()
{
  CleanupSwapChainResources();
  CleanupDeviceResources();

  m_hWnd = nullptr;
  m_uiWidth = 0;
  m_uiHeight = 0;
}

bool nsDirectX11Renderer::IsInitialized() const
{
  return m_pDevice != nullptr && m_pContext != nullptr;
}

nsResult nsDirectX11Renderer::CreateDevice()
{
  UINT createDeviceFlags = 0;
  if (m_bEnableDebugLayer)
  {
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  }

  D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
  };

  D3D_FEATURE_LEVEL featureLevel;
  HRESULT hr = D3D11CreateDevice(
    nullptr,                    // Use default adapter
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,                    // No software device
    createDeviceFlags,
    featureLevels,
    NS_ARRAY_SIZE(featureLevels),
    D3D11_SDK_VERSION,
    &m_pDevice,
    &featureLevel,
    &m_pContext);

  if (FAILED(hr))
  {
    nsLog::Error("Failed to create D3D11 device (HRESULT: 0x{0})", nsArgU(static_cast<nsUInt32>(hr), 8, true, 16, true));
    return NS_FAILURE;
  }

  nsLog::Info("Created D3D11 device with feature level {0}.{1}",
    (featureLevel >> 12) & 0xF,
    (featureLevel >> 8) & 0xF);

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateSwapChain(HWND hWnd, nsUInt32 uiWidth, nsUInt32 uiHeight)
{
  ComPtr<IDXGIDevice> pDxgiDevice;
  HRESULT hr = m_pDevice.As(&pDxgiDevice);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to get DXGI device");
    return NS_FAILURE;
  }

  ComPtr<IDXGIAdapter> pAdapter;
  hr = pDxgiDevice->GetAdapter(&pAdapter);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to get DXGI adapter");
    return NS_FAILURE;
  }

  ComPtr<IDXGIFactory> pFactory;
  hr = pAdapter->GetParent(__uuidof(IDXGIFactory), &pFactory);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to get DXGI factory");
    return NS_FAILURE;
  }

  DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
  swapChainDesc.BufferCount = 2;
  swapChainDesc.BufferDesc.Width = uiWidth;
  swapChainDesc.BufferDesc.Height = uiHeight;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = hWnd;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  hr = pFactory->CreateSwapChain(m_pDevice.Get(), &swapChainDesc, &m_pSwapChain);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create swap chain (HRESULT: 0x{0})", nsArgU(static_cast<nsUInt32>(hr), 8, true, 16, true));
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateRenderTargetView()
{
  ComPtr<ID3D11Texture2D> pBackBuffer;
  HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to get back buffer");
    return NS_FAILURE;
  }

  hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRenderTargetView);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create render target view");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateDepthStencilView()
{
  D3D11_TEXTURE2D_DESC depthDesc = {};
  depthDesc.Width = m_uiWidth;
  depthDesc.Height = m_uiHeight;
  depthDesc.MipLevels = 1;
  depthDesc.ArraySize = 1;
  depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthDesc.SampleDesc.Count = 1;
  depthDesc.SampleDesc.Quality = 0;
  depthDesc.Usage = D3D11_USAGE_DEFAULT;
  depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

  HRESULT hr = m_pDevice->CreateTexture2D(&depthDesc, nullptr, &m_pDepthStencilBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create depth stencil buffer");
    return NS_FAILURE;
  }

  hr = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), nullptr, &m_pDepthStencilView);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create depth stencil view");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateShaders()
{
  nsDynamicArray<char> vertexSource;
  NS_SUCCEED_OR_RETURN(LoadShaderSource(s_szVertexShaderPath, vertexSource));

  nsDynamicArray<char> pixelSource;
  NS_SUCCEED_OR_RETURN(LoadShaderSource(s_szPixelShaderPath, pixelSource));

  ComPtr<ID3DBlob> pVSBlob;
  ComPtr<ID3DBlob> pErrorBlob;

  HRESULT hr = D3DCompile(
    vertexSource.GetData(),
    vertexSource.GetCount() - 1,
    s_szVertexShaderPath,
    nullptr,
    nullptr,
    "mainVS",
    "vs_5_0",
    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
    0,
    &pVSBlob,
    &pErrorBlob);

  if (FAILED(hr))
  {
    if (pErrorBlob)
    {
      nsLog::Error("Vertex shader compilation failed: {0}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
    }
    else
    {
      nsLog::Error("Vertex shader compilation failed (HRESULT: 0x{0})", nsArgU(static_cast<nsUInt32>(hr), 8, true, 16, true));
    }
    return NS_FAILURE;
  }

  hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create vertex shader");
    return NS_FAILURE;
  }

  ComPtr<ID3DBlob> pPSBlob;
  hr = D3DCompile(
    pixelSource.GetData(),
    pixelSource.GetCount() - 1,
    s_szPixelShaderPath,
    nullptr,
    nullptr,
    "mainPS",
    "ps_5_0",
    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
    0,
    &pPSBlob,
    &pErrorBlob);

  if (FAILED(hr))
  {
    if (pErrorBlob)
    {
      nsLog::Error("Pixel shader compilation failed: {0}", static_cast<const char*>(pErrorBlob->GetBufferPointer()));
    }
    else
    {
      nsLog::Error("Pixel shader compilation failed (HRESULT: 0x{0})", nsArgU(static_cast<nsUInt32>(hr), 8, true, 16, true));
    }
    return NS_FAILURE;
  }

  hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create pixel shader");
    return NS_FAILURE;
  }

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  hr = m_pDevice->CreateInputLayout(
    layout,
    NS_ARRAY_SIZE(layout),
    pVSBlob->GetBufferPointer(),
    pVSBlob->GetBufferSize(),
    &m_pInputLayout);

  if (FAILED(hr))
  {
    nsLog::Error("Failed to create input layout");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateInputLayout()
{
  // Already created in CreateShaders
  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateConstantBuffers()
{
  D3D11_BUFFER_DESC cbDesc = {};
  cbDesc.ByteWidth = sizeof(ConstantBufferData);
  cbDesc.Usage = D3D11_USAGE_DYNAMIC;
  cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  HRESULT hr = m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create constant buffer");
    return NS_FAILURE;
  }

  cbDesc.ByteWidth = sizeof(PushConstantData);
  hr = m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pInstanceConstantBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create instance constant buffer");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateGeometryBuffers()
{
  static constexpr float s_cubeVertices[] = {
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
  };

  static constexpr nsUInt16 s_cubeIndices[] = {
    0, 1, 2, 2, 3, 0, // back
    4, 5, 6, 6, 7, 4, // front
    4, 5, 1, 1, 0, 4, // bottom
    7, 6, 2, 2, 3, 7, // top
    5, 6, 2, 2, 1, 5, // right
    4, 7, 3, 3, 0, 4  // left
  };

  m_uiIndexCount = NS_ARRAY_SIZE(s_cubeIndices);

  D3D11_BUFFER_DESC vbDesc = {};
  vbDesc.ByteWidth = sizeof(s_cubeVertices);
  vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
  vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vbData = {};
  vbData.pSysMem = s_cubeVertices;

  HRESULT hr = m_pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create vertex buffer");
    return NS_FAILURE;
  }

  D3D11_BUFFER_DESC ibDesc = {};
  ibDesc.ByteWidth = sizeof(s_cubeIndices);
  ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
  ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA ibData = {};
  ibData.pSysMem = s_cubeIndices;

  hr = m_pDevice->CreateBuffer(&ibDesc, &ibData, &m_pIndexBuffer);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create index buffer");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateRasterizerState()
{
  D3D11_RASTERIZER_DESC rasterizerDesc = {};
  rasterizerDesc.FillMode = D3D11_FILL_SOLID;
  rasterizerDesc.CullMode = D3D11_CULL_BACK;
  rasterizerDesc.FrontCounterClockwise = TRUE;
  rasterizerDesc.DepthClipEnable = TRUE;

  HRESULT hr = m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create rasterizer state");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateBlendState()
{
  D3D11_BLEND_DESC blendDesc = {};
  blendDesc.RenderTarget[0].BlendEnable = FALSE;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  HRESULT hr = m_pDevice->CreateBlendState(&blendDesc, &m_pBlendState);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create blend state");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

nsResult nsDirectX11Renderer::CreateDepthStencilState()
{
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
  depthStencilDesc.DepthEnable = TRUE;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

  HRESULT hr = m_pDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to create depth stencil state");
    return NS_FAILURE;
  }

  return NS_SUCCESS;
}

void nsDirectX11Renderer::SetBackBufferSize(nsUInt32 uiWidth, nsUInt32 uiHeight)
{
  if (uiWidth == 0 || uiHeight == 0)
    return;

  if (uiWidth == m_uiWidth && uiHeight == m_uiHeight)
    return;

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_bResizePending = true;
}

nsResult nsDirectX11Renderer::RecreateSwapChain()
{
  if (!m_bResizePending)
    return NS_SUCCESS;

  if (!m_pSwapChain || !m_pContext)
    return NS_FAILURE;

  m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
  m_pRenderTargetView.Reset();
  m_pDepthStencilView.Reset();
  m_pDepthStencilBuffer.Reset();

  HRESULT hr = m_pSwapChain->ResizeBuffers(0, m_uiWidth, m_uiHeight, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr))
  {
    nsLog::Error("Failed to resize swap chain buffers");
    return NS_FAILURE;
  }

  NS_SUCCEED_OR_RETURN(CreateRenderTargetView());
  NS_SUCCEED_OR_RETURN(CreateDepthStencilView());

  m_bResizePending = false;
  return NS_SUCCESS;
}

void nsDirectX11Renderer::UpdateScene(const nsMat4& mViewProjection, nsArrayPtr<const nsDirectX11InstanceData> instances)
{
  m_viewProjection = mViewProjection;
  m_sceneInstances.SetCount(instances.GetCount());
  if (!instances.IsEmpty())
  {
    nsMemoryUtils::Copy(m_sceneInstances.GetData(), instances.GetPtr(), instances.GetCount());
  }
}

nsResult nsDirectX11Renderer::RenderFrame()
{
  if (!m_pDevice || !m_pContext)
    return NS_FAILURE;

  if (m_bResizePending)
  {
    NS_SUCCEED_OR_RETURN(RecreateSwapChain());
  }

  // Clear
  const float clearColor[] = {0.02f, 0.05f, 0.09f, 1.0f};
  m_pContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clearColor);
  m_pContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  // Set render target
  m_pContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

  // Set viewport
  D3D11_VIEWPORT viewport = {};
  viewport.Width = static_cast<float>(m_uiWidth);
  viewport.Height = static_cast<float>(m_uiHeight);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  m_pContext->RSSetViewports(1, &viewport);

  // Set shaders and states
  m_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
  m_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
  m_pContext->IASetInputLayout(m_pInputLayout.Get());
  m_pContext->RSSetState(m_pRasterizerState.Get());
  m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
  m_pContext->OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
  m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Update constant buffer
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  if (SUCCEEDED(hr))
  {
    ConstantBufferData* pCBData = static_cast<ConstantBufferData*>(mappedResource.pData);
    pCBData->m_ViewProjection = m_viewProjection;
    m_pContext->Unmap(m_pConstantBuffer.Get(), 0);
  }

  m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

  // Set vertex and index buffers
  UINT stride = sizeof(float) * 3;
  UINT offset = 0;
  m_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
  m_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

  // Draw ground grid for spatial reference (20x20 grid, 1 unit spacing)
  {
    const float gridSize = 20.0f;
    const float gridSpacing = 1.0f;
    const float lineThickness = 0.02f;
    const nsColor gridColor(0.3f, 0.3f, 0.3f, 1.0f);
    const int gridLines = static_cast<int>(gridSize / gridSpacing);

    // Draw lines along X-axis (parallel to X)
    for (int i = -gridLines; i <= gridLines; ++i)
    {
      float yPos = i * gridSpacing;
      nsMat4 xform = nsMat4::MakeTranslation(nsVec3(0.0f, yPos, 0.0f)) * nsMat4::MakeScaling(nsVec3(gridSize, lineThickness, lineThickness));

      hr = m_pContext->Map(m_pInstanceConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
      if (SUCCEEDED(hr))
      {
        PushConstantData* pPushData = static_cast<PushConstantData*>(mappedResource.pData);
        pPushData->m_Model = xform;
        pPushData->m_Color = gridColor;
        m_pContext->Unmap(m_pInstanceConstantBuffer.Get(), 0);
      }

      m_pContext->VSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());
      m_pContext->PSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());
      m_pContext->DrawIndexed(m_uiIndexCount, 0, 0);
    }

    // Draw lines along Y-axis (parallel to Y)
    for (int i = -gridLines; i <= gridLines; ++i)
    {
      float xPos = i * gridSpacing;
      nsMat4 xform = nsMat4::MakeTranslation(nsVec3(xPos, 0.0f, 0.0f)) * nsMat4::MakeScaling(nsVec3(lineThickness, gridSize, lineThickness));

      hr = m_pContext->Map(m_pInstanceConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
      if (SUCCEEDED(hr))
      {
        PushConstantData* pPushData = static_cast<PushConstantData*>(mappedResource.pData);
        pPushData->m_Model = xform;
        pPushData->m_Color = gridColor;
        m_pContext->Unmap(m_pInstanceConstantBuffer.Get(), 0);
      }

      m_pContext->VSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());
      m_pContext->PSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());
      m_pContext->DrawIndexed(m_uiIndexCount, 0, 0);
    }
  }

  // Draw instances
  for (const nsDirectX11InstanceData& instance : m_sceneInstances)
  {
    if (instance.m_bSleeping)
      continue;

    hr = m_pContext->Map(m_pInstanceConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
      PushConstantData* pPushData = static_cast<PushConstantData*>(mappedResource.pData);
      pPushData->m_Model = instance.m_ModelMatrix;
      pPushData->m_Color = instance.m_Color;
      m_pContext->Unmap(m_pInstanceConstantBuffer.Get(), 0);
    }

    m_pContext->VSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());
    m_pContext->PSSetConstantBuffers(1, 1, m_pInstanceConstantBuffer.GetAddressOf());

    m_pContext->DrawIndexed(m_uiIndexCount, 0, 0);
  }

  // Present
  m_pSwapChain->Present(1, 0);

  return NS_SUCCESS;
}

void nsDirectX11Renderer::CleanupSwapChainResources()
{
  if (m_pContext)
  {
    m_pContext->ClearState();
    m_pContext->Flush();
  }

  m_pRenderTargetView.Reset();
  m_pDepthStencilView.Reset();
  m_pDepthStencilBuffer.Reset();
  m_pSwapChain.Reset();
}

void nsDirectX11Renderer::CleanupDeviceResources()
{
  m_pVertexBuffer.Reset();
  m_pIndexBuffer.Reset();
  m_pConstantBuffer.Reset();
  m_pInstanceConstantBuffer.Reset();
  m_pInputLayout.Reset();
  m_pVertexShader.Reset();
  m_pPixelShader.Reset();
  m_pRasterizerState.Reset();
  m_pBlendState.Reset();
  m_pDepthStencilState.Reset();

  if (m_pContext)
  {
    m_pContext->ClearState();
    m_pContext->Flush();
  }

  m_pContext.Reset();
  m_pDevice.Reset();
}

#endif // NS_ENABLED(NS_PLATFORM_WINDOWS)

NS_STATICLINK_FILE(DirectX11Renderer, DirectX11Renderer_DirectX11RendererModule);
