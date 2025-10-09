#include <VulkanRendererTest/VulkanRendererTestPCH.h>

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Types/ScopeExit.h>

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#endif

#include <dxcapi.h>

#if NS_ENABLED(NS_PLATFORM_LINUX) || NS_ENABLED(NS_PLATFORM_OSX)
#  include <dlfcn.h>
#endif

namespace
{
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
  using CreateInstanceProc = HRESULT(__stdcall*)(REFCLSID, REFIID, void**);
#else
  using CreateInstanceProc = HRESULT (*)(REFCLSID, REFIID, void**);
#endif

  nsResult ResolveDxcCreateInstance(CreateInstanceProc& outProc)
  {
#if NS_ENABLED(NS_PLATFORM_WINDOWS)
    outProc = &DxcCreateInstance;
    return NS_SUCCESS;
#else
    static CreateInstanceProc s_pCreateInstance = nullptr;
    static void* s_pDxcLibraryHandle = nullptr;

    outProc = nullptr;

    if (s_pCreateInstance != nullptr)
    {
      outProc = s_pCreateInstance;
      return NS_SUCCESS;
    }

    nsHybridArray<nsString, 8> candidateLibraries;

    nsStringBuilder envOverride;
    if (nsEnvironmentVariableUtils::GetValueString("DXC_LIBRARY_PATH", envOverride).Succeeded() && !envOverride.IsEmpty())
    {
      candidateLibraries.ExpandAndGetRef() = envOverride;
    }

#  if NS_ENABLED(NS_PLATFORM_OSX)
    candidateLibraries.PushBack("libdxcompiler.dylib");
    candidateLibraries.PushBack("/usr/local/lib/libdxcompiler.dylib");
    candidateLibraries.PushBack("/opt/homebrew/lib/libdxcompiler.dylib");
#  else
    candidateLibraries.PushBack("libdxcompiler.so");
    candidateLibraries.PushBack("libdxcompiler.so.3");
    candidateLibraries.PushBack("libdxcompiler.so.3.7");
    candidateLibraries.PushBack("/usr/lib/libdxcompiler.so");
    candidateLibraries.PushBack("/usr/local/lib/libdxcompiler.so");
#  endif

    for (const nsString& candidate : candidateLibraries)
    {
      if (candidate.IsEmpty())
        continue;

      dlerror();
      void* pLibraryHandle = dlopen(candidate.GetData(), RTLD_LAZY | RTLD_LOCAL);
      if (pLibraryHandle == nullptr)
      {
        const char* szError = dlerror();
        if (szError != nullptr)
        {
          nsLog::Debug("DXC: Failed to load '{0}': {1}", candidate, szError);
        }
        continue;
      }

      void* pSymbol = dlsym(pLibraryHandle, "DxcCreateInstance");
      if (pSymbol == nullptr)
      {
        nsLog::Warning("DXC: '{0}' does not export DxcCreateInstance", candidate);
        dlclose(pLibraryHandle);
        continue;
      }

      s_pDxcLibraryHandle = pLibraryHandle;
      s_pCreateInstance = reinterpret_cast<CreateInstanceProc>(pSymbol);

      static struct DxcLibraryShutdown
      {
        ~DxcLibraryShutdown()
        {
          if (s_pDxcLibraryHandle != nullptr)
          {
            dlclose(s_pDxcLibraryHandle);
            s_pDxcLibraryHandle = nullptr;
          }
        }
      } s_Shutdown;

      outProc = s_pCreateInstance;
      return NS_SUCCESS;
    }

    nsLog::Error("DXC runtime library could not be located. Set DXC_LIBRARY_PATH or install libdxcompiler.");
    return NS_FAILURE;
#endif
  }

  nsResult LoadShaderSource(nsStringView relativeFileName, nsDynamicArray<char>& outBuffer)
  {
    nsStringBuilder relativePath("Data/Base/Shaders/VulkanRenderer");
    relativePath.AppendPath(relativeFileName);

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

    TryOpen(relativePath.GetView());

    nsHybridArray<nsString, 4> searchRoots;
    nsStringBuilder root = nsOSFile::GetApplicationDirectory();
    root.MakeCleanPath();
    for (nsUInt32 i = 0; i < 4 && !root.IsEmpty(); ++i)
    {
      searchRoots.PushBack(root.GetView());

      const nsUInt32 lengthBefore = root.GetCharacterCount();
      root.PathParentDirectory();
      root.MakeCleanPath();

      if (root.IsEmpty() || root.GetCharacterCount() >= lengthBefore)
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

    if (openedPath.IsEmpty())
    {
      nsStringBuilder attemptsList;
      for (nsUInt32 i = 0; i < attemptedPaths.GetCount(); ++i)
      {
        if (i > 0)
          attemptsList.Append(", ");
        attemptsList.Append(attemptedPaths[i]);
      }

      nsLog::Error("Failed to open shader file '{0}' (relative to working directory). Tried: {1}", relativePath, attemptsList);
      return NS_FAILURE;
    }

    const nsUInt64 fileSize = file.GetFileSize();
    outBuffer.SetCount(static_cast<nsUInt32>(fileSize + 1));

    const nsUInt64 bytesRead = file.ReadBytes(outBuffer.GetData(), fileSize);
    if (bytesRead != fileSize)
    {
      nsLog::Error("Failed to read shader file '{0}'", openedPath);
      return NS_FAILURE;
    }

    outBuffer[static_cast<nsUInt32>(fileSize)] = '\0';
    return NS_SUCCESS;
  }

  nsResult CompileShader(nsStringView relativeFileName, CreateInstanceProc createInstance, const wchar_t* entryPoint, const wchar_t* shaderProfile)
  {
    NS_ASSERT_DEV(createInstance != nullptr, "DXC create instance function must be valid");

    nsDynamicArray<char> shaderSource;
    NS_SUCCEED_OR_RETURN(LoadShaderSource(relativeFileName, shaderSource));
    if (shaderSource.GetCount() <= 1)
    {
      nsLog::Error("Shader source '{0}' is empty", relativeFileName);
      return NS_FAILURE;
    }

    nsArrayPtr<const char> sourceView(shaderSource.GetData(), shaderSource.GetCount() - 1);

    auto hresultHex = [](HRESULT value) {
      return nsArgU(static_cast<nsUInt64>(static_cast<nsUInt32>(value)), 8, true, 16, true);
    };

    IDxcUtils* pUtils = nullptr;
    HRESULT hr = createInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), reinterpret_cast<void**>(&pUtils));
    if (FAILED(hr) || pUtils == nullptr)
    {
      nsLog::Error("Failed to create DxcUtils instance for '{0}' (HRESULT: 0x{1})", relativeFileName, hresultHex(hr));
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pUtils) { pUtils->Release(); });

    IDxcCompiler3* pCompiler = nullptr;
    hr = createInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(&pCompiler));
    if (FAILED(hr) || pCompiler == nullptr)
    {
      nsLog::Error("Failed to create DxcCompiler instance for '{0}' (HRESULT: 0x{1})", relativeFileName, hresultHex(hr));
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pCompiler) { pCompiler->Release(); });

    IDxcIncludeHandler* pIncludeHandler = nullptr;
    hr = pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
    if (FAILED(hr) || pIncludeHandler == nullptr)
    {
      nsLog::Error("Failed to create Dxc include handler (HRESULT: 0x{0})", hresultHex(hr));
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pIncludeHandler) { pIncludeHandler->Release(); });

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = sourceView.GetPtr();
    sourceBuffer.Size = static_cast<SIZE_T>(sourceView.GetCount());
    sourceBuffer.Encoding = DXC_CP_UTF8;

    const wchar_t* arguments[] = {
      L"-E", entryPoint,
      L"-T", shaderProfile,
      L"-spirv",
      L"-fspv-target-env=vulkan1.2",
      L"-fvk-use-dx-layout",
      L"-O0"
    };

    IDxcResult* pResult = nullptr;
    HRESULT compileHr = pCompiler->Compile(&sourceBuffer, arguments, NS_ARRAY_SIZE(arguments), pIncludeHandler, __uuidof(IDxcResult), reinterpret_cast<void**>(&pResult));
    if (FAILED(compileHr) || pResult == nullptr)
    {
      nsLog::Error("Failed to compile shader '{0}' with DXC (HRESULT: 0x{1})", relativeFileName, hresultHex(compileHr));
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pResult) { pResult->Release(); });

    HRESULT status = S_OK;
    if (FAILED(pResult->GetStatus(&status)) || FAILED(status))
    {
      IDxcBlobUtf8* pErrors = nullptr;
      if (SUCCEEDED(pResult->GetOutput(DXC_OUT_ERRORS, __uuidof(IDxcBlobUtf8), reinterpret_cast<void**>(&pErrors), nullptr)) && pErrors != nullptr)
      {
        if (pErrors->GetStringLength() > 0)
        {
          nsLog::Error("DXC compilation errors for '{0}': {1}", relativeFileName, pErrors->GetStringPointer());
        }
        pErrors->Release();
      }
      return NS_FAILURE;
    }

    IDxcBlob* pObject = nullptr;
    if (FAILED(pResult->GetOutput(DXC_OUT_OBJECT, __uuidof(IDxcBlob), reinterpret_cast<void**>(&pObject), nullptr)) || pObject == nullptr)
    {
      nsLog::Error("Failed to retrieve compiled shader blob for '{0}'", relativeFileName);
      return NS_FAILURE;
    }
    NS_SCOPE_EXIT(if (pObject) { pObject->Release(); });

    if (pObject->GetBufferSize() == 0)
    {
      nsLog::Error("Compiled shader blob for '{0}' is empty", relativeFileName);
      return NS_FAILURE;
    }

    return NS_SUCCESS;
  }
}

NS_CREATE_SIMPLE_TEST_GROUP(VulkanRenderer);

NS_CREATE_SIMPLE_TEST(VulkanRenderer, ShaderSmokeTest)
{
  CreateInstanceProc createInstance = nullptr;
  NS_TEST_RESULT(ResolveDxcCreateInstance(createInstance));
  NS_TEST_BOOL(createInstance != nullptr);

  NS_TEST_RESULT(CompileShader("PvdSceneVS.hlsl", createInstance, L"mainVS", L"vs_6_0"));
  NS_TEST_RESULT(CompileShader("PvdScenePS.hlsl", createInstance, L"mainPS", L"ps_6_0"));
}
