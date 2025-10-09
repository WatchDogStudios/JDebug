#include <Core/CorePCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsCurve1DResource, 1, nsRTTIDefaultAllocator<nsCurve1DResource>)
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_RESOURCE_IMPLEMENT_COMMON_CODE(nsCurve1DResource);

nsCurve1DResource::nsCurve1DResource()
  : nsResource(DoUpdate::OnAnyThread, 1)
{
}

NS_RESOURCE_IMPLEMENT_CREATEABLE(nsCurve1DResource, nsCurve1DResourceDescriptor)
{
  m_Descriptor = descriptor;

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Loaded;

  return res;
}

nsResourceLoadDesc nsCurve1DResource::UnloadData(Unload WhatToUnload)
{
  NS_IGNORE_UNUSED(WhatToUnload);

  nsResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = nsResourceState::Unloaded;

  m_Descriptor.m_Curves.Clear();

  return res;
}

nsResourceLoadDesc nsCurve1DResource::UpdateContent(nsStreamReader* Stream)
{
  NS_LOG_BLOCK("nsCurve1DResource::UpdateContent", GetResourceIdOrDescription());

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

void nsCurve1DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<nsUInt32>(m_Descriptor.m_Curves.GetHeapMemoryUsage()) + static_cast<nsUInt32>(sizeof(m_Descriptor));

  for (const auto& curve : m_Descriptor.m_Curves)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += curve.GetHeapMemoryUsage();
  }
}

void nsCurve1DResourceDescriptor::Save(nsStreamWriter& inout_stream) const
{
  const nsUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  const nsUInt8 uiCurves = static_cast<nsUInt8>(m_Curves.GetCount());
  inout_stream << uiCurves;

  for (nsUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Save(inout_stream);
  }
}

void nsCurve1DResourceDescriptor::Load(nsStreamReader& inout_stream)
{
  nsUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  NS_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  nsUInt8 uiCurves = 0;
  inout_stream >> uiCurves;

  m_Curves.SetCount(uiCurves);

  for (nsUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Load(inout_stream);

    /// \todo We can do this on load, or somehow ensure this is always already correctly saved
    m_Curves[i].SortControlPoints();
    m_Curves[i].CreateLinearApproximation();
  }
}



NS_STATICLINK_FILE(Core, Core_Curves_Implementation_Curve1DResource);
