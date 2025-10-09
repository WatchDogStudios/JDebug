#include <Core/CorePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsColorGradientResource, 1, nsRTTIDefaultAllocator<nsColorGradientResource>)
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_RESOURCE_IMPLEMENT_COMMON_CODE(nsColorGradientResource);

nsColorGradientResource::nsColorGradientResource()
  : nsResource(DoUpdate::OnAnyThread, 1)
{
}

NS_RESOURCE_IMPLEMENT_CREATEABLE(nsColorGradientResource, nsColorGradientResourceDescriptor)
{
  m_Descriptor = descriptor;

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Loaded;

  return res;
}

nsResourceLoadDesc nsColorGradientResource::UnloadData(Unload WhatToUnload)
{
  NS_IGNORE_UNUSED(WhatToUnload);

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

nsResourceLoadDesc nsColorGradientResource::UpdateContent(nsStreamReader* Stream)
{
  NS_LOG_BLOCK("nsColorGradientResource::UpdateContent", GetResourceIdOrDescription());

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

  m_Descriptor.Load(*Stream);

  res.m_State = nsResourceState::Loaded;
  return res;
}

void nsColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<nsUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<nsUInt32>(sizeof(m_Descriptor));
}

void nsColorGradientResourceDescriptor::Save(nsStreamWriter& inout_stream) const
{
  const nsUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Gradient.Save(inout_stream);
}

void nsColorGradientResourceDescriptor::Load(nsStreamReader& inout_stream)
{
  nsUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  NS_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(inout_stream);
}



NS_STATICLINK_FILE(Core, Core_Curves_Implementation_ColorGradientResource);
