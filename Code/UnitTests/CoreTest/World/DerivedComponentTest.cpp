#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  using TestComponentBaseManager = nsComponentManagerSimple<class TestComponentBase, nsComponentUpdateType::Always>;

  class TestComponentBase : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(TestComponentBase, nsComponent, TestComponentBaseManager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentBase::s_iUpdateCounter = 0;

  NS_BEGIN_COMPONENT_TYPE(TestComponentBase, 1, nsComponentMode::Static)
  NS_END_COMPONENT_TYPE

  //////////////////////////////////////////////////////////////////////////

  using TestComponentDerived1Manager = nsComponentManagerSimple<class TestComponentDerived1, nsComponentUpdateType::Always>;

  class TestComponentDerived1 : public TestComponentBase
  {
    NS_DECLARE_COMPONENT_TYPE(TestComponentDerived1, TestComponentBase, TestComponentDerived1Manager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentDerived1::s_iUpdateCounter = 0;

  NS_BEGIN_COMPONENT_TYPE(TestComponentDerived1, 1, nsComponentMode::Static)
  NS_END_COMPONENT_TYPE
} // namespace


NS_CREATE_SIMPLE_TEST(World, DerivedComponents)
{
  nsWorldDesc worldDesc("Test");
  nsWorld world(worldDesc);
  NS_LOCK(world.GetWriteMarker());

  TestComponentBaseManager* pManagerBase = world.GetOrCreateComponentManager<TestComponentBaseManager>();
  TestComponentDerived1Manager* pManagerDerived1 = world.GetOrCreateComponentManager<TestComponentDerived1Manager>();

  nsGameObjectDesc desc;
  nsGameObject* pObject;
  nsGameObjectHandle hObject = world.CreateObject(desc, pObject);
  NS_TEST_BOOL(!hObject.IsInvalidated());

  nsGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponentBase::s_iUpdateCounter = 0;
  TestComponentDerived1::s_iUpdateCounter = 0;

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Derived Component Update")
  {
    TestComponentBase* pComponentBase = nullptr;
    nsTypedComponentHandle<TestComponentBase> hComponentBase = TestComponentBase::CreateComponent(pObject, pComponentBase);

    TestComponentBase* pTestBase = nullptr;
    NS_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestBase));
    NS_TEST_BOOL(pTestBase == pComponentBase);
    NS_TEST_BOOL(pComponentBase->GetHandle() == hComponentBase);
    NS_TEST_BOOL(pComponentBase->GetOwningManager() == pManagerBase);

    TestComponentDerived1* pComponentDerived1 = nullptr;
    nsTypedComponentHandle<TestComponentDerived1> hComponentDerived1 = TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1);

    TestComponentDerived1* pTestDerived1 = nullptr;
    NS_TEST_BOOL(world.TryGetComponent(hComponentDerived1, pTestDerived1));
    NS_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    NS_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentDerived1);
    NS_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    world.Update();

    NS_TEST_INT(TestComponentBase::s_iUpdateCounter, 1);
    NS_TEST_INT(TestComponentDerived1::s_iUpdateCounter, 1);

    // Get component manager via rtti
    NS_TEST_BOOL(world.GetManagerForComponentType(nsGetStaticRTTI<TestComponentBase>()) == pManagerBase);
    NS_TEST_BOOL(world.GetManagerForComponentType(nsGetStaticRTTI<TestComponentDerived1>()) == pManagerDerived1);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Derived Component TypedComponentHandle Assign")
  {
    TestComponentDerived1* pComponentDerived1 = nullptr;
    nsTypedComponentHandle<TestComponentBase> hComponentBase(TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1));

    TestComponentDerived1* pTestDerived1 = nullptr;
    NS_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestDerived1));
    NS_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    NS_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentBase);
    NS_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    // Assignment test Derived -> Base
    hComponentBase = pTestDerived1->GetHandle();
    NS_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestDerived1));
    NS_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentBase);
    NS_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);
  }
}
