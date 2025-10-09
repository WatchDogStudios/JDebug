#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
NS_BEGIN_COMPONENT_TYPE(nsCollectionComponent, 2, nsComponentMode::Static)
{
  NS_BEGIN_PROPERTIES
  {
    NS_RESOURCE_ACCESSOR_PROPERTY("Collection", GetCollection, SetCollection)->AddAttributes(new nsAssetBrowserAttribute("CompatibleAsset_AssetCollection", nsDependencyFlags::Package)),
    NS_MEMBER_PROPERTY("RegisterNames", m_bRegisterNames),
  }
  NS_END_PROPERTIES;
  NS_BEGIN_ATTRIBUTES
  {
    new nsCategoryAttribute("Utilities"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

nsCollectionComponent::nsCollectionComponent() = default;
nsCollectionComponent::~nsCollectionComponent() = default;

void nsCollectionComponent::SerializeComponent(nsWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollection;
  s << m_bRegisterNames;
}

void nsCollectionComponent::DeserializeComponent(nsWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const nsUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hCollection;

  if (uiVersion >= 2)
  {
    s >> m_bRegisterNames;
  }
}

void nsCollectionComponent::SetCollection(const nsCollectionResourceHandle& hCollection)
{
  m_hCollection = hCollection;

  if (IsActiveAndSimulating())
  {
    InitiatePreload();
  }
}

void nsCollectionComponent::OnSimulationStarted()
{
  InitiatePreload();
}

void nsCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    nsResourceLock<nsCollectionResource> pCollection(m_hCollection, nsResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pCollection.GetAcquireResult() == nsResourceAcquireResult::Final)
    {
      pCollection->PreloadResources();

      if (m_bRegisterNames)
      {
        pCollection->RegisterNames();
      }
    }
  }
}

NS_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
