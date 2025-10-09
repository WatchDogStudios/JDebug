#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  class nsTestComponentManager;

  class nsTestComponent : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(nsTestComponent, nsComponent, nsTestComponentManager);
  };

  class nsTestComponentManager : public nsComponentManager<class nsTestComponent, nsBlockStorageType::FreeList>
  {
  public:
    nsTestComponentManager(nsWorld* pWorld)
      : nsComponentManager<nsTestComponent, nsBlockStorageType::FreeList>(pWorld)
    {
      m_qRotation.SetIdentity();
    }

    virtual void Initialize() override
    {
      auto desc = nsWorldModule::UpdateFunctionDesc(nsWorldModule::UpdateFunction(&nsTestComponentManager::Update, this), "Update");
      desc.m_bOnlyUpdateWhenSimulating = false;

      RegisterUpdateFunction(desc);
    }

    void Update(const nsWorldModule::UpdateContext& context)
    {
      nsQuat qRot = nsQuat::MakeFromAxisAndAngle(nsVec3(0, 0, 1), nsAngle::MakeFromDegree(2.0f));

      m_qRotation = qRot * m_qRotation;

      for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
      {
        ComponentType* pComponent = it;
        if (pComponent->IsActiveAndInitialized())
        {
          auto pOwner = pComponent->GetOwner();
          pOwner->SetLocalRotation(m_qRotation);
        }
      }
    }

    nsQuat m_qRotation;
  };

  // clang-format off
  NS_BEGIN_COMPONENT_TYPE(nsTestComponent, 1, nsComponentMode::Dynamic);
  NS_END_COMPONENT_TYPE;
  // clang-format on

  void AddObjectsToWorld(nsWorld& ref_world, bool bDynamic, nsUInt32 uiNumObjects, nsUInt32 uiTreeLevelNumNodeDiv, nsUInt32 uiTreeDepth,
    nsInt32 iAttachCompsDepth, nsGameObjectHandle hParent = nsGameObjectHandle())
  {
    if (uiTreeDepth == 0)
      return;

    nsGameObjectDesc gd;
    gd.m_bDynamic = bDynamic;
    gd.m_hParent = hParent;

    float posX = 0.0f;
    float posY = uiTreeDepth * 5.0f;

    nsTestComponentManager* pMan = ref_world.GetOrCreateComponentManager<nsTestComponentManager>();

    for (nsUInt32 i = 0; i < uiNumObjects; ++i)
    {
      gd.m_LocalPosition.Set(posX, posY, 0);
      posX += 5.0f;

      nsGameObject* pObj;
      auto hObj = ref_world.CreateObject(gd, pObj);

      if (iAttachCompsDepth > 0)
      {
        nsTestComponent* comp;
        pMan->CreateComponent(pObj, comp);
      }

      AddObjectsToWorld(
        ref_world, bDynamic, nsMath::Max(uiNumObjects / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(
    bool bDynamic, nsUInt32 uiNumObjects, nsUInt32 uiTreeLevelNumNodeDiv, nsUInt32 uiTreeDepth, nsInt32 iAttachCompsDepth, nsWorld* pWorld = nullptr)
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);

    if (pWorld == nullptr)
    {
      pWorld = &world;
    }

    NS_LOCK(pWorld->GetWriteMarker());

    {
      nsStopwatch sw;

      AddObjectsToWorld(*pWorld, bDynamic, uiNumObjects, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Creating %u %s objects (depth: %u): %.2fms", pWorld->GetObjectCount(),
        bDynamic ? "dynamic" : "static", uiTreeDepth, tDiff.GetMilliseconds());
    }
  }

} // namespace


#if NS_ENABLED(NS_COMPILE_FOR_DEBUG)
static const nsTestBlock::Enum EnableInRelease = nsTestBlock::DisabledNoWarning;
#else
static const nsTestBlock::Enum EnableInRelease = nsTestBlock::Enabled;
#endif

NS_CREATE_SIMPLE_TEST(World, Profile_Creation)
{
  NS_TEST_BLOCK(EnableInRelease, "Create many objects")
  {
    // it makes no difference whether we create static or dynamic objects
    static bool bDynamic = true;
    bDynamic = !bDynamic;

    MeasureCreationTime(bDynamic, 10, 1, 4, 0);
    MeasureCreationTime(bDynamic, 10, 1, 5, 0);
    MeasureCreationTime(bDynamic, 100, 1, 2, 0);
    MeasureCreationTime(bDynamic, 10000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 1000000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100, 1, 3, 0);
    MeasureCreationTime(bDynamic, 3, 1, 12, 0);
    MeasureCreationTime(bDynamic, 1, 1, 80, 0);
  }
}

NS_CREATE_SIMPLE_TEST(World, Profile_Deletion)
{
  NS_TEST_BLOCK(EnableInRelease, "Delete many objects")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    nsStopwatch sw;

    NS_LOCK(world.GetWriteMarker());
    nsUInt32 uiNumObjects = world.GetObjectCount();

    world.Clear();
    world.Update();

    const nsTime tDiff = sw.Checkpoint();
    nsTestFramework::Output(nsTestOutput::Duration, "Deleting %u objects: %.2fms", uiNumObjects, tDiff.GetMilliseconds());
  }
}

NS_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  NS_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static objects")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    MeasureCreationTime(false, 100, 1, 3, 0, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  NS_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 0, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  NS_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects with components")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  NS_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic objects")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  NS_TEST_BLOCK(EnableInRelease, "MT Update 250,000 dynamic objects")
  {
    nsWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  NS_TEST_BLOCK(EnableInRelease, "MT Update 1,000,000 dynamic objects")
  {
    nsWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    nsWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    nsStopwatch sw;

    // first round always has some overhead
    for (nsUInt32 i = 0; i < 3; ++i)
    {
      NS_LOCK(world.GetWriteMarker());
      world.Update();

      const nsTime tDiff = sw.Checkpoint();

      nsTestFramework::Output(nsTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
