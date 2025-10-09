#pragma once

#include <Core/Collection/CollectionResource.h>
#include <Core/CoreDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using nsCollectionComponentManager = nsComponentManager<class nsCollectionComponent, nsBlockStorageType::Compact>;

/// \brief An nsCollectionComponent references an nsCollectionResource and triggers resource preloading when needed
///
/// Placing an nsCollectionComponent in a scene or a model makes it possible to tell the engine to preload certain resources
/// that are likely to be needed soon.
///
/// If a deactivated nsCollectionComponent is part of the scene, it will not trigger a preload, but will do so once
/// the component is activated.
class NS_CORE_DLL nsCollectionComponent : public nsComponent
{
  NS_DECLARE_COMPONENT_TYPE(nsCollectionComponent, nsComponent, nsCollectionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // nsComponent
public:
  virtual void SerializeComponent(nsWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(nsWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // nsCollectionComponent
public:
  nsCollectionComponent();
  ~nsCollectionComponent();

  void SetCollection(const nsCollectionResourceHandle& hPrefab);                                     // [ property ]
  NS_ALWAYS_INLINE const nsCollectionResourceHandle& GetCollection() const { return m_hCollection; } // [ property ]

protected:
  /// \brief Triggers the preload on the referenced nsCollectionResource
  void InitiatePreload();

  bool m_bRegisterNames = false;
  nsCollectionResourceHandle m_hCollection;
};
