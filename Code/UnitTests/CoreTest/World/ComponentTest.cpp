#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public nsComponentManager<TestComponent, nsBlockStorageType::FreeList>
  {
  public:
    TestComponentManager(nsWorld* pWorld)
      : nsComponentManager<TestComponent, nsBlockStorageType::FreeList>(pWorld)
    {
    }

    virtual void Initialize() override
    {
      auto desc = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2 = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      auto desc3 = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update3, this);
      desc3.m_fPriority = 1000.0f;

      auto desc4 = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::AUpdate3, this);
      desc4.m_fPriority = 1000.0f;

      desc.m_DependsOn.PushBack(nsMakeHashedString("TestComponentManager::Update2")); // update2 will be called before update
      desc.m_DependsOn.PushBack(nsMakeHashedString("TestComponentManager::Update3")); // update3 will be called before update

      auto descAsync = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase = nsWorldUpdatePhase::Async;
      descAsync.m_uiAsyncPhaseBatchSize = 20;

      // Update functions are now registered in reverse order, so we can test whether dependencies work.
      this->RegisterUpdateFunction(descAsync);
      this->RegisterUpdateFunction(desc4);
      this->RegisterUpdateFunction(desc3);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(desc);
    }

    void Update(const nsWorldModule::UpdateContext& context);
    void Update2(const nsWorldModule::UpdateContext& context);
    void Update3(const nsWorldModule::UpdateContext& context);
    void AUpdate3(const nsWorldModule::UpdateContext& context);
    void UpdateAsync(const nsWorldModule::UpdateContext& context);
  };

  class TestComponent : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(TestComponent, nsComponent, TestComponentManager);

  public:
    TestComponent() = default;
    ~TestComponent() = default;

    virtual void Initialize() override { ++s_iInitCounter; }
    virtual void Deinitialize() override { --s_iInitCounter; }

    virtual void OnActivated() override
    {
      ++s_iActivateCounter;

      SpawnOther();
    }

    virtual void OnDeactivated() override { --s_iActivateCounter; }

    virtual void OnSimulationStarted() override { ++s_iSimulationStartedCounter; }

    void Update() { m_iSomeData *= 5; }

    void Update2() { m_iSomeData += 3; }

    void SpawnOther();

    nsInt32 m_iSomeData = 1;

    static nsInt32 s_iInitCounter;
    static nsInt32 s_iActivateCounter;
    static nsInt32 s_iSimulationStartedCounter;

    static bool s_bSpawnOther;
  };

  nsInt32 TestComponent::s_iInitCounter = 0;
  nsInt32 TestComponent::s_iActivateCounter = 0;
  nsInt32 TestComponent::s_iSimulationStartedCounter = 0;
  bool TestComponent::s_bSpawnOther = false;

  NS_BEGIN_COMPONENT_TYPE(TestComponent, 1, nsComponentMode::Static)
  NS_END_COMPONENT_TYPE

  void TestComponentManager::Update(const nsWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(const nsWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::Update3(const nsWorldModule::UpdateContext& context) {}

  void TestComponentManager::AUpdate3(const nsWorldModule::UpdateContext& context) {}

  void TestComponentManager::UpdateAsync(const nsWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  using TestComponent2Manager = nsComponentManager<class TestComponent2, nsBlockStorageType::FreeList>;

  class TestComponent2 : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(TestComponent2, nsComponent, TestComponent2Manager);

    virtual void OnActivated() override { TestComponent::s_iActivateCounter++; }
  };

  NS_BEGIN_COMPONENT_TYPE(TestComponent2, 1, nsComponentMode::Static)
  NS_END_COMPONENT_TYPE

  void TestComponent::SpawnOther()
  {
    if (s_bSpawnOther)
    {
      nsGameObjectDesc desc;
      desc.m_hParent = GetOwner()->GetHandle();

      nsGameObject* pChild = nullptr;
      GetWorld()->CreateObject(desc, pChild);

      TestComponent2* pChildComponent = nullptr;
      TestComponent2::CreateComponent(pChild, pChildComponent);
    }
  }
} // namespace


NS_CREATE_SIMPLE_TEST(World, Components)
{
  nsWorldDesc worldDesc("Test");
  nsWorld world(worldDesc);
  NS_LOCK(world.GetWriteMarker());

  TestComponentManager* pManager = world.GetOrCreateComponentManager<TestComponentManager>();

  nsGameObject* pTestObject1;
  nsGameObject* pTestObject2;

  {
    nsGameObjectDesc desc;
    nsGameObjectHandle hObject = world.CreateObject(desc, pTestObject1);
    NS_TEST_BOOL(!hObject.IsInvalidated());
    world.CreateObject(desc, pTestObject2);
  }

  TestComponent* pTestComponent = nullptr;

  TestComponent::s_iInitCounter = 0;
  TestComponent::s_iActivateCounter = 0;
  TestComponent::s_iSimulationStartedCounter = 0;
  TestComponent::s_bSpawnOther = false;

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Component Init")
  {
    // test recursive write lock
    NS_LOCK(world.GetWriteMarker());

    nsTypedComponentHandle<TestComponent> handle;
    NS_TEST_BOOL(!world.TryGetComponent(handle, pTestComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(pTestObject1, pTestComponent);

    TestComponent* pTest = nullptr;
    NS_TEST_BOOL(world.TryGetComponent(handle, pTest));
    NS_TEST_BOOL(pTest == pTestComponent);
    NS_TEST_BOOL(pTestComponent->GetHandle() == handle);

    TestComponent2* pTest2 = nullptr;
    NS_TEST_BOOL(!world.TryGetComponent(nsComponentHandle(handle), pTest2));

    NS_TEST_INT(pTestComponent->m_iSomeData, 1);
    NS_TEST_INT(TestComponent::s_iInitCounter, 0);

    for (nsUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pTestObject2, pTestComponent);
      pTestComponent->m_iSomeData = i + 1;
    }

    NS_TEST_INT(pManager->GetComponentCount(), 100);
    NS_TEST_INT(TestComponent::s_iInitCounter, 0);

    // Update with components created
    world.Update();

    NS_TEST_INT(pManager->GetComponentCount(), 100);
    NS_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Component Update")
  {
    // test recursive read lock
    NS_LOCK(world.GetReadMarker());

    world.Update();

    nsUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      NS_TEST_INT(it->m_iSomeData, (((uiCounter + 4) * 25) + 3) * 25);
      ++uiCounter;
    }

    NS_TEST_INT(uiCounter, 100);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pTestComponent->GetHandle());
    NS_TEST_INT(pManager->GetComponentCount(), 99);
    NS_TEST_INT(TestComponent::s_iInitCounter, 99);

    // component should also be removed from the game object
    NS_TEST_INT(pTestObject2->GetComponents().GetCount(), 98);

    world.DeleteObjectNow(pTestObject2->GetHandle());
    world.Update();

    NS_TEST_INT(TestComponent::s_iInitCounter, 1);

    world.DeleteComponentManager<TestComponentManager>();
    pManager = nullptr;
    NS_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Delete Objects with Component")
  {
    nsGameObjectDesc desc;

    nsGameObject* pObjectA = nullptr;
    nsGameObject* pObjectB = nullptr;
    nsGameObject* pObjectC = nullptr;

    desc.m_sName.Assign("A");
    nsGameObjectHandle hObjectA = world.CreateObject(desc, pObjectA);
    desc.m_sName.Assign("B");
    nsGameObjectHandle hObjectB = world.CreateObject(desc, pObjectB);
    desc.m_sName.Assign("C");
    nsGameObjectHandle hObjectC = world.CreateObject(desc, pObjectC);

    NS_TEST_BOOL(!hObjectA.IsInvalidated());
    NS_TEST_BOOL(!hObjectB.IsInvalidated());
    NS_TEST_BOOL(!hObjectC.IsInvalidated());

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    nsTypedComponentHandle<TestComponent> hComponentA = TestComponent::CreateComponent(pObjectA, pComponentA);
    nsTypedComponentHandle<TestComponent> hComponentB = TestComponent::CreateComponent(pObjectB, pComponentB);
    nsTypedComponentHandle<TestComponent> hComponentC = TestComponent::CreateComponent(pObjectC, pComponentC);

    NS_TEST_BOOL(!hComponentA.IsInvalidated());
    NS_TEST_BOOL(!hComponentB.IsInvalidated());
    NS_TEST_BOOL(!hComponentC.IsInvalidated());

    world.DeleteObjectNow(pObjectB->GetHandle());

    NS_TEST_BOOL(pObjectA->IsActive());
    NS_TEST_BOOL(pComponentA->IsActive());
    NS_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    NS_TEST_BOOL(!pObjectB->IsActive());
    NS_TEST_BOOL(!pComponentB->IsActive());
    NS_TEST_BOOL(pComponentB->GetOwner() == nullptr);

    NS_TEST_BOOL(pObjectC->IsActive());
    NS_TEST_BOOL(pComponentC->IsActive());
    NS_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    world.Update();

    NS_TEST_BOOL(world.TryGetObject(hObjectA, pObjectA));
    NS_TEST_BOOL(world.TryGetObject(hObjectC, pObjectC));

    // Since we're not recompacting storage for components, pointer should still be valid.
    // NS_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    // NS_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    NS_TEST_BOOL(pObjectA->IsActive());
    NS_TEST_BOOL(pObjectA->GetName() == "A");
    NS_TEST_BOOL(pComponentA->IsActive());
    NS_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    NS_TEST_BOOL(pObjectC->IsActive());
    NS_TEST_BOOL(pObjectC->GetName() == "C");
    NS_TEST_BOOL(pComponentC->IsActive());
    NS_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent* pComponentB2 = nullptr;
    nsTypedComponentHandle<TestComponent> hComponentB2 = TestComponent::CreateComponent(pObjectB, pComponentB2);
    NS_TEST_BOOL(!hComponentB2.IsInvalidated());
    NS_TEST_BOOL(pComponentB2 == pComponentB);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Get Components")
  {
    const nsWorld& constWorld = world;

    const TestComponentManager* pConstManager = constWorld.GetComponentManager<TestComponentManager>();

    for (auto it = pConstManager->GetComponents(); it.IsValid(); it.Next())
    {
      nsTypedComponentHandle<TestComponent> hComponent = it->GetHandle();

      const TestComponent* pConstComponent = nullptr;
      NS_TEST_BOOL(constWorld.TryGetComponent(hComponent, pConstComponent));
      NS_TEST_BOOL(pConstComponent == (const TestComponent*)it);

      NS_TEST_BOOL(pConstManager->TryGetComponent(hComponent, pConstComponent));
      NS_TEST_BOOL(pConstComponent == (const TestComponent*)it);
    }

    world.DeleteComponentManager<TestComponentManager>();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Component Callbacks")
  {
    nsGameObjectDesc desc;
    nsGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    // Simulation stopped, component active
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      NS_TEST_INT(TestComponent::s_iInitCounter, 0);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(false);

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 2);

      pComponent->DeleteComponent();
    }

    // Simulation stopped, component inactive
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      NS_TEST_INT(TestComponent::s_iInitCounter, 0);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(false);

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component active
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      NS_TEST_INT(TestComponent::s_iInitCounter, 0);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }

    // Simulation started, component inactive
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      NS_TEST_INT(TestComponent::s_iInitCounter, 0);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 0);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      NS_TEST_INT(TestComponent::s_iInitCounter, 1);
      NS_TEST_INT(TestComponent::s_iActivateCounter, 1);
      NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->DeleteComponent();
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Component dependent initialization")
  {
    nsGameObjectDesc desc;
    nsGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    world.SetWorldSimulationEnabled(true);

    TestComponent::s_iInitCounter = 0;
    TestComponent::s_iActivateCounter = 0;
    TestComponent::s_iSimulationStartedCounter = 0;
    TestComponent::s_bSpawnOther = true;

    TestComponent* pComponent = nullptr;
    TestComponent::CreateComponent(pObject, pComponent);

    world.Update();

    NS_TEST_INT(TestComponent::s_iInitCounter, 1);
    NS_TEST_INT(TestComponent::s_iActivateCounter, 2);
    NS_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);
  }
}
