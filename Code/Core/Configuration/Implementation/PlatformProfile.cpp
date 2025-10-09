#include <Core/CorePCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#include <Core/ResourceManager/ResourceManager.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsProfileConfigData, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

nsProfileConfigData::nsProfileConfigData() = default;
nsProfileConfigData::~nsProfileConfigData() = default;

void nsProfileConfigData::SaveRuntimeData(nsChunkStreamWriter& inout_stream) const
{
  NS_IGNORE_UNUSED(inout_stream);
}

void nsProfileConfigData::LoadRuntimeData(nsChunkStreamReader& inout_stream)
{
  NS_IGNORE_UNUSED(inout_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsPlatformProfile, 1, nsRTTIDefaultAllocator<nsPlatformProfile>)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new nsHiddenAttribute()),
    NS_MEMBER_PROPERTY("TargetPlatform", m_sTargetPlatform)->AddAttributes(new nsDynamicStringEnumAttribute("TargetPlatformNames"), new nsDefaultValueAttribute("Windows")),
    NS_ARRAY_MEMBER_PROPERTY("Configs", m_Configs)->AddFlags(nsPropertyFlags::PointerOwner)->AddAttributes(new nsContainerAttribute(false, false, false)),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

nsPlatformProfile::nsPlatformProfile() = default;

nsPlatformProfile::~nsPlatformProfile()
{
  Clear();
}

void nsPlatformProfile::Clear()
{
  for (auto pType : m_Configs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_Configs.Clear();
}

void nsPlatformProfile::AddMissingConfigs()
{
  nsRTTI::ForEachDerivedType<nsProfileConfigData>(
    [this](const nsRTTI* pRtti)
    {
      // find all types derived from nsProfileConfigData
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType && pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        nsProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<nsProfileConfigData>();
        NS_ASSERT_DEV(pObject != nullptr, "Invalid profile config");
        nsReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    },
    nsRTTI::ForEachOptions::ExcludeNonAllocatable);

  // in case unknown configs were loaded from disk, remove them
  m_Configs.RemoveAndSwap(nullptr);

  // sort all configs alphabetically
  m_Configs.Sort([](const nsProfileConfigData* lhs, const nsProfileConfigData* rhs) -> bool
    { return lhs->GetDynamicRTTI()->GetTypeName().Compare(rhs->GetDynamicRTTI()->GetTypeName()) < 0; });
}

const nsProfileConfigData* nsPlatformProfile::GetTypeConfig(const nsRTTI* pRtti) const
{
  for (const auto* pConfig : m_Configs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

nsProfileConfigData* nsPlatformProfile::GetTypeConfig(const nsRTTI* pRtti)
{
  // reuse the const-version
  return const_cast<nsProfileConfigData*>(((const nsPlatformProfile*)this)->GetTypeConfig(pRtti));
}

nsResult nsPlatformProfile::SaveForRuntime(nsStringView sFile) const
{
  nsFileWriter file;
  NS_SUCCEED_OR_RETURN(file.Open(sFile));

  nsChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return NS_SUCCESS;
}

nsResult nsPlatformProfile::LoadForRuntime(nsStringView sFile)
{
  nsFileReader file;
  NS_SUCCEED_OR_RETURN(file.Open(sFile));

  nsChunkStreamReader chunk(file);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    for (auto* pConfig : m_Configs)
    {
      pConfig->LoadRuntimeData(chunk);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  ++m_uiLastModificationCounter;
  return NS_SUCCESS;
}



NS_STATICLINK_FILE(Core, Core_Configuration_Implementation_PlatformProfile);
