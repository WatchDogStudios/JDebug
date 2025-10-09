#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

/// \brief Descriptor for color gradient resources containing the gradient data and serialization methods.
struct NS_CORE_DLL nsColorGradientResourceDescriptor
{
  nsColorGradient m_Gradient;

  void Save(nsStreamWriter& inout_stream) const;
  void Load(nsStreamReader& inout_stream);
};

using nsColorGradientResourceHandle = nsTypedResourceHandle<class nsColorGradientResource>;

/// \brief A resource that stores a single color gradient for use in rendering and effects.
///
/// Color gradient resources allow artists to define color transitions that can be evaluated
/// at runtime. Commonly used for particle effects, UI elements, and other visual systems
/// that need smooth color transitions.
class NS_CORE_DLL nsColorGradientResource : public nsResource
{
  NS_ADD_DYNAMIC_REFLECTION(nsColorGradientResource, nsResource);
  NS_RESOURCE_DECLARE_COMMON_CODE(nsColorGradientResource);
  NS_RESOURCE_DECLARE_CREATEABLE(nsColorGradientResource, nsColorGradientResourceDescriptor);

public:
  nsColorGradientResource();

  /// \brief Returns all the data that is stored in this resource.
  const nsColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  /// \brief Evaluates the color gradient at the given position and returns the interpolated color.
  inline nsColor Evaluate(double x) const
  {
    nsColor result;
    m_Descriptor.m_Gradient.Evaluate(x, result);
    return result;
  }

private:
  virtual nsResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual nsResourceLoadDesc UpdateContent(nsStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  nsColorGradientResourceDescriptor m_Descriptor;
};
