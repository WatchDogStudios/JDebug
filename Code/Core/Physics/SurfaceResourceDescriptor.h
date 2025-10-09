#pragma once

#include <Core/CoreDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using nsSurfaceResourceHandle = nsTypedResourceHandle<class nsSurfaceResource>;
using nsPrefabResourceHandle = nsTypedResourceHandle<class nsPrefabResource>;


/// \brief Defines how prefabs are aligned when spawned during surface interactions.
struct nsSurfaceInteractionAlignment
{
  using StorageType = nsUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsSurfaceInteractionAlignment);


/// \brief Describes how a surface responds to a specific type of interaction.
///
/// Configures the prefab to spawn, its alignment, impact thresholds, and custom parameters
/// when objects interact with a surface in a particular way (collision, slide, roll, etc.).
struct NS_CORE_DLL nsSurfaceInteraction
{
  nsString m_sInteractionType;

  nsPrefabResourceHandle m_hPrefab;
  nsEnum<nsSurfaceInteractionAlignment> m_Alignment;
  nsAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;

  const nsRangeView<const char*, nsUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const nsVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, nsVariant& out_value) const; // [ property ] (exposed parameter)

  nsArrayMap<nsHashedString, nsVariant> m_Parameters;
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsSurfaceInteraction);

/// \brief Descriptor containing all configuration data for a surface resource.
///
/// Defines physics properties (restitution, friction), interaction behaviors,
/// base surface inheritance, and navigation ground type information.
struct NS_CORE_DLL nsSurfaceResourceDescriptor : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsSurfaceResourceDescriptor, nsReflectedClass);

public:
  void Load(nsStreamReader& inout_stream);
  void Save(nsStreamWriter& inout_stream) const;

  void SetCollisionInteraction(const char* szName);
  const char* GetCollisionInteraction() const;

  void SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;

  nsSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  nsHashedString m_sOnCollideInteraction;
  nsHashedString m_sSlideInteractionPrefab;
  nsHashedString m_sRollInteractionPrefab;
  nsInt8 m_iGroundType = -1; ///< What kind of ground this is for navigation purposes. Ground type properties need to be specified elsewhere, this is just a number.

  nsHybridArray<nsSurfaceInteraction, 16> m_Interactions;
};
