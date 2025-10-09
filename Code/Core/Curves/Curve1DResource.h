#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// \brief Descriptor for 1D curve resources containing multiple curves and serialization methods.
///
/// A curve resource can contain more than one curve, but all curves are of the same type.
/// This allows grouping related curves together for efficiency and logical organization.
struct NS_CORE_DLL nsCurve1DResourceDescriptor
{
  nsDynamicArray<nsCurve1D> m_Curves;

  void Save(nsStreamWriter& inout_stream) const;
  void Load(nsStreamReader& inout_stream);
};

using nsCurve1DResourceHandle = nsTypedResourceHandle<class nsCurve1DResource>;

/// \brief A resource that stores multiple 1D curves for animation and value interpolation.
///
/// 1D curve resources contain mathematical curves that map time or other input values to
/// output values. Commonly used for animations, easing functions, and procedural value
/// generation where smooth interpolation is needed.
class NS_CORE_DLL nsCurve1DResource : public nsResource
{
  NS_ADD_DYNAMIC_REFLECTION(nsCurve1DResource, nsResource);
  NS_RESOURCE_DECLARE_COMMON_CODE(nsCurve1DResource);
  NS_RESOURCE_DECLARE_CREATEABLE(nsCurve1DResource, nsCurve1DResourceDescriptor);

public:
  nsCurve1DResource();

  /// \brief Returns all the data that is stored in this resource.
  const nsCurve1DResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual nsResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual nsResourceLoadDesc UpdateContent(nsStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  nsCurve1DResourceDescriptor m_Descriptor;
};
