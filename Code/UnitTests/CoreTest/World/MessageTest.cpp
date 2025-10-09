#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>

namespace
{
  struct nsMsgTest : public nsMessage
  {
    NS_DECLARE_MESSAGE_TYPE(nsMsgTest, nsMessage);
  };

  // clang-format off
  NS_IMPLEMENT_MESSAGE_TYPE(nsMsgTest);
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsMsgTest, 1, nsRTTIDefaultAllocator<nsMsgTest>)
  NS_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  struct TestMessage1 : public nsMsgTest
  {
    NS_DECLARE_MESSAGE_TYPE(TestMessage1, nsMsgTest);

    int m_iValue;
  };

  struct TestMessage2 : public nsMsgTest
  {
    NS_DECLARE_MESSAGE_TYPE(TestMessage2, nsMsgTest);

    virtual nsInt32 GetSortingKey() const override { return 2; }

    int m_iValue;
  };

  // clang-format off
  NS_IMPLEMENT_MESSAGE_TYPE(TestMessage1);
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage1, 1, nsRTTIDefaultAllocator<TestMessage1>)
  NS_END_DYNAMIC_REFLECTED_TYPE;

  NS_IMPLEMENT_MESSAGE_TYPE(TestMessage2);
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage2, 1, nsRTTIDefaultAllocator<TestMessage2>)
  NS_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestComponentMsg;
  using TestComponentMsgManager = nsComponentManager<TestComponentMsg, nsBlockStorageType::FreeList>;

  class TestComponentMsg : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(TestComponentMsg, nsComponent, TestComponentMsgManager);

  public:
    TestComponentMsg()

      = default;
    ~TestComponentMsg() = default;

    virtual void SerializeComponent(nsWorldWriter& inout_stream) const override {}
    virtual void DeserializeComponent(nsWorldReader& inout_stream) override {}

    void OnTestMessage(TestMessage1& ref_msg) { m_iSomeData += ref_msg.m_iValue; }

    void OnTestMessage2(TestMessage2& ref_msg) { m_iSomeData2 += 2 * ref_msg.m_iValue; }

    nsInt32 m_iSomeData = 1;
    nsInt32 m_iSomeData2 = 2;
  };

  // clang-format off
  NS_BEGIN_COMPONENT_TYPE(TestComponentMsg, 1, nsComponentMode::Static)
  {
    NS_BEGIN_MESSAGEHANDLERS
    {
      NS_MESSAGE_HANDLER(TestMessage1, OnTestMessage),
      NS_MESSAGE_HANDLER(TestMessage2, OnTestMessage2),
    }
    NS_END_MESSAGEHANDLERS;
  }
  NS_END_COMPONENT_TYPE;
  // clang-format on

  void ResetComponents(nsGameObject& ref_object)
  {
    TestComponentMsg* pComponent = nullptr;
    if (ref_object.TryGetComponentOfBaseType(pComponent))
    {
      pComponent->m_iSomeData = 1;
      pComponent->m_iSomeData2 = 2;
    }

    for (auto it = ref_object.GetChildren(); it.IsValid(); ++it)
    {
      ResetComponents(*it);
    }
  }
} // namespace

NS_CREATE_SIMPLE_TEST(World, Messaging)
{
  nsWorldDesc worldDesc("Test");
  nsWorld world(worldDesc);
  NS_LOCK(world.GetWriteMarker());

  TestComponentMsgManager* pManager = world.GetOrCreateComponentManager<TestComponentMsgManager>();

  nsGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  nsGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  TestComponentMsg* pComponent = nullptr;
  pManager->CreateComponent(pRoot, pComponent);

  nsGameObject* pParents[2];
  desc.m_hParent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pManager->CreateComponent(pParents[0], pComponent);

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pManager->CreateComponent(pParents[1], pComponent);

  for (nsUInt32 i = 0; i < 2; ++i)
  {
    desc.m_hParent = pParents[i]->GetHandle();
    for (nsUInt32 j = 0; j < 4; ++j)
    {
      nsStringBuilder sb;
      sb.AppendFormat("Parent{0}_Child{1}", i + 1, j + 1);
      desc.m_sName.Assign(sb.GetData());

      nsGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pManager->CreateComponent(pObject, pComponent);
    }
  }

  // one update step so components are initialized
  world.Update();

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage1 msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2);

    TestComponentMsg* pComponent2 = nullptr;
    NS_TEST_BOOL(pParents[0]->TryGetComponentOfBaseType(pComponent2));
    NS_TEST_INT(pComponent2->m_iSomeData, 5);
    NS_TEST_INT(pComponent2->m_iSomeData2, 10);

    // siblings, parent and children should not be affected
    NS_TEST_BOOL(pParents[1]->TryGetComponentOfBaseType(pComponent2));
    NS_TEST_INT(pComponent2->m_iSomeData, 1);
    NS_TEST_INT(pComponent2->m_iSomeData2, 2);

    NS_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
    NS_TEST_INT(pComponent2->m_iSomeData, 1);
    NS_TEST_INT(pComponent2->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      NS_TEST_BOOL(it->TryGetComponentOfBaseType(pComponent2));
      NS_TEST_INT(pComponent2->m_iSomeData, 1);
      NS_TEST_INT(pComponent2->m_iSomeData2, 2);
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, nsTime::MakeZero(), nsObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, nsTime::MakeZero(), nsObjectMsgQueueType::NextFrame);
    }

    world.Update();

    TestComponentMsg* pComponent2 = nullptr;
    NS_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
    NS_TEST_INT(pComponent2->m_iSomeData, 46);
    NS_TEST_INT(pComponent2->m_iSomeData2, 92);

    nsFrameAllocator::Reset();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, nsTime::MakeFromSeconds(i + 1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, nsTime::MakeFromSeconds(i + 1));
    }

    world.GetClock().SetFixedTimeStep(nsTime::MakeFromSeconds(1.001f));

    int iDesiredValue = 1;
    int iDesiredValue2 = 2;

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      world.Update();

      TestComponentMsg* pComponent2 = nullptr;
      NS_TEST_BOOL(pRoot->TryGetComponentOfBaseType(pComponent2));
      NS_TEST_INT(pComponent2->m_iSomeData, iDesiredValue);
      NS_TEST_INT(pComponent2->m_iSomeData2, iDesiredValue2);
    }

    nsFrameAllocator::Reset();
  }
}
