#include <Core/CorePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/AssetFileHeader.h>

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsCollectionResource, 1, nsRTTIDefaultAllocator<nsCollectionResource>)
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_RESOURCE_IMPLEMENT_COMMON_CODE(nsCollectionResource);

nsCollectionResource::nsCollectionResource()
  : nsResource(DoUpdate::OnAnyThread, 1)
{
}

// UnloadData() already makes sure to call UnregisterNames();
nsCollectionResource::~nsCollectionResource() = default;

bool nsCollectionResource::PreloadResources(nsUInt32 uiNumResourcesToPreload)
{
  NS_LOCK(m_PreloadMutex);
  NS_PROFILE_SCOPE("Inject Resources to Preload");

  if (m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount())
  {
    // All resources have already been queued so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return false;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  const nsUInt32 remainingResources = m_Collection.m_Resources.GetCount() - m_PreloadedResources.GetCount();
  const nsUInt32 end = nsMath::Min(remainingResources, uiNumResourcesToPreload) + m_PreloadedResources.GetCount();
  for (nsUInt32 i = m_PreloadedResources.GetCount(); i < end; ++i)
  {
    const nsCollectionEntry& e = m_Collection.m_Resources[i];
    nsTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const nsRTTI* pRtti = nsResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = nsResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        nsLog::Warning("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register the resource type with the nsResourceManager?", e.m_sAssetTypeName, nsArgSensitive(e.m_sResourceID, "ResourceID"));
      }
    }
    else
    {
      nsLog::Error("Asset '{}' had an empty asset type name. Cannot pre-load it.", nsArgSensitive(e.m_sResourceID, "ResourceID"));
    }

    m_PreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      nsResourceManager::PreloadResource(hTypeless);
    }
  }

  return m_PreloadedResources.GetCount() < m_Collection.m_Resources.GetCount();
}

bool nsCollectionResource::IsLoadingFinished(float* out_pProgress) const
{
  NS_LOCK(m_PreloadMutex);

  nsUInt64 loadedWeight = 0;
  nsUInt64 totalWeight = 0;

  nsUInt32 uiPoked = 0;

  for (nsUInt32 i = 0; i < m_PreloadedResources.GetCount(); i++)
  {
    const nsTypelessResourceHandle& hResource = m_PreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const nsCollectionEntry& entry = m_Collection.m_Resources[i];
    nsUInt64 thisWeight = nsMath::Max(entry.m_uiFileSize, 1ull); // if file sizes are not specified, we weight by 1
    nsResourceState state = nsResourceManager::GetLoadingState(hResource);

    if (state == nsResourceState::Loaded || state == nsResourceState::LoadedResourceMissing)
    {
      loadedWeight += thisWeight;
    }
    else if (state != nsResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
    else
    {
      if (uiPoked < 3)
      {
        // there's a bug or race condition somewhere when unloading resources, which means resources that should be queued
        // for preloading don't get preloaded and then the entire preloading system gets stuck
        // to prevent this, we'll make sure that the next few unloaded resources do get requeued for preload

        ++uiPoked;
        nsResourceManager::PreloadResource(hResource);
      }
    }
  }

  if (out_pProgress != nullptr)
  {
    const float maxLoadedFraction = m_Collection.m_Resources.GetCount() == 0 ? 1.f : (float)m_PreloadedResources.GetCount() / m_Collection.m_Resources.GetCount();
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_pProgress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight) * maxLoadedFraction;
    }
    else
    {
      *out_pProgress = maxLoadedFraction;
    }
  }

  if (totalWeight == 0 || totalWeight == loadedWeight)
  {
    return true;
  }

  return false;
}


const nsCollectionResourceDescriptor& nsCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

NS_RESOURCE_IMPLEMENT_CREATEABLE(nsCollectionResource, nsCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Loaded;

  return res;
}

nsResourceLoadDesc nsCollectionResource::UnloadData(Unload WhatToUnload)
{
  NS_IGNORE_UNUSED(WhatToUnload);

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Unloaded;

  {
    UnregisterNames();
    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    // NS_LOCK(m_preloadMutex);
    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

nsResourceLoadDesc nsCollectionResource::UpdateContent(nsStreamReader* Stream)
{
  NS_LOG_BLOCK("nsCollectionResource::UpdateContent", GetResourceIdOrDescription());

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = nsResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  nsStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  nsAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Collection.Load(*Stream);

  res.m_State = nsResourceState::Loaded;
  return res;
}

void nsCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  NS_LOCK(m_PreloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<nsUInt32>(m_PreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
}


void nsCollectionResource::RegisterNames()
{
  if (m_bRegistered)
    return;

  m_bRegistered = true;

  NS_LOCK(nsResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      nsResourceManager::RegisterNamedResource(entry.m_sOptionalNiceLookupName, entry.m_sResourceID);
    }
  }
}


void nsCollectionResource::UnregisterNames()
{
  if (!m_bRegistered)
    return;

  m_bRegistered = false;

  NS_LOCK(nsResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      nsResourceManager::UnregisterNamedResource(entry.m_sOptionalNiceLookupName);
    }
  }
}

void nsCollectionResourceDescriptor::Save(nsStreamWriter& inout_stream) const
{
  const nsUInt8 uiVersion = 3;
  const nsUInt8 uiIdentifier = 0xC0;
  const nsUInt32 uiNumResources = m_Resources.GetCount();

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << uiNumResources;

  for (nsUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream << m_Resources[i].m_sAssetTypeName;
    inout_stream << m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream << m_Resources[i].m_sResourceID;
    inout_stream << m_Resources[i].m_uiFileSize;
  }
}

void nsCollectionResourceDescriptor::Load(nsStreamReader& inout_stream)
{
  nsUInt8 uiVersion = 0;
  nsUInt8 uiIdentifier = 0;
  nsUInt32 uiNumResources = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    nsUInt16 uiNumResourcesShort;
    inout_stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    inout_stream >> uiNumResources;
  }

  NS_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid nsCollectionResourceDescriptor");
  NS_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (nsUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream >> m_Resources[i].m_sAssetTypeName;
    inout_stream >> m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      inout_stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



NS_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);
