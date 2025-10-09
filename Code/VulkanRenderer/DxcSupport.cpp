#include <VulkanRenderer/VulkanRendererPCH.h>
#include <VulkanRenderer/DxcSupport.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/System/EnvironmentVariableUtils.h>

#if NS_ENABLED(NS_PLATFORM_LINUX) || NS_ENABLED(NS_PLATFORM_OSX)
#  include <dlfcn.h>
#elif NS_ENABLED(NS_PLATFORM_WINDOWS)
#  include <Foundation/Basics/IncludeAll.h>
#endif

namespace nsVulkanDxc
{
  nsResult ResolveCreateInstance(CreateInstanceProc& outProc)
  {
    outProc = nullptr;

    static CreateInstanceProc s_pCreateInstance = nullptr;

#if NS_ENABLED(NS_PLATFORM_WINDOWS)
    static HMODULE s_hDxcLibraryHandle = nullptr;

    if (s_pCreateInstance != nullptr)
    {
      outProc = s_pCreateInstance;
      return NS_SUCCESS;
    }

    nsHybridArray<nsString, 8> candidateLibraries;

    nsString envOverride = nsEnvironmentVariableUtils::GetValueString("DXC_LIBRARY_PATH");
    if (!envOverride.IsEmpty())
    {
      candidateLibraries.PushBack(envOverride);
    }

    envOverride = nsEnvironmentVariableUtils::GetValueString("VULKAN_SDK");
    if (!envOverride.IsEmpty())
    {
      nsStringBuilder vulkanSdkPath = envOverride;
      vulkanSdkPath.AppendPath("bin");
      vulkanSdkPath.AppendPath("dxcompiler.dll");
      candidateLibraries.PushBack(vulkanSdkPath);
    }

    candidateLibraries.PushBack("dxcompiler.dll");

    for (const nsString& candidate : candidateLibraries)
    {
      if (candidate.IsEmpty())
        continue;

      nsStringBuilder resolvedCandidate = candidate;
      if (!resolvedCandidate.EndsWith_NoCase(".dll"))
      {
        resolvedCandidate.AppendPath("dxcompiler.dll");
      }

      nsStringWChar widePath(resolvedCandidate);
      HMODULE hModule = ::LoadLibraryW(widePath.GetData());
      if (hModule == nullptr)
      {
        nsLog::Debug("DXC: Failed to load '{0}' (Win32 error {1})", resolvedCandidate, static_cast<nsUInt32>(::GetLastError()));
        continue;
      }

      FARPROC pSymbol = ::GetProcAddress(hModule, "DxcCreateInstance");
      if (pSymbol == nullptr)
      {
        nsLog::Warning("DXC: '{0}' does not export DxcCreateInstance", resolvedCandidate);
        ::FreeLibrary(hModule);
        continue;
      }

      s_hDxcLibraryHandle = hModule;
      s_pCreateInstance = reinterpret_cast<CreateInstanceProc>(pSymbol);

      static struct DxcLibraryShutdown
      {
        ~DxcLibraryShutdown()
        {
          if (s_hDxcLibraryHandle != nullptr)
          {
            ::FreeLibrary(s_hDxcLibraryHandle);
            s_hDxcLibraryHandle = nullptr;
          }
        }
      } s_Shutdown;

      outProc = s_pCreateInstance;
      return NS_SUCCESS;
    }

    nsLog::Error("DXC runtime library could not be located. Set DXC_LIBRARY_PATH or install dxcompiler.dll.");
    return NS_FAILURE;
#else
    static void* s_pDxcLibraryHandle = nullptr;

    if (s_pCreateInstance != nullptr)
    {
      outProc = s_pCreateInstance;
      return NS_SUCCESS;
    }

    nsHybridArray<nsString, 8> candidateLibraries;

    nsString envOverride = nsEnvironmentVariableUtils::GetValueString("DXC_LIBRARY_PATH");
    if (!envOverride.IsEmpty())
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
}
