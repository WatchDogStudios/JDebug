#include <CoreTest/CoreTestPCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>

NS_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects
  {
    struct
    {
      nsGameObject* pParent1;
      nsGameObject* pParent2;
      nsGameObject* pChild11;
      nsGameObject* pChild21;
    };
    nsGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(nsWorld& ref_world, bool bDynamic)
  {
    TestWorldObjects testWorldObjects;
    nsMemoryUtils::ZeroFill(&testWorldObjects, 1);

    nsQuat q = nsQuat::MakeFromAxisAndAngle(nsVec3(0.0f, 0.0f, 1.0f), nsAngle::MakeFromDegree(90.0f));

    nsGameObjectDesc desc;
    desc.m_bDynamic = bDynamic;
    desc.m_LocalPosition = nsVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = nsVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    ref_world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    ref_world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_hParent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    ref_world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_hParent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    ref_world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o, nsVec3 vOffset = nsVec3(100.0f, 0.0f, 0.0f))
  {
    const float eps = nsMath::DefaultEpsilon<float>();
    nsQuat q = nsQuat::MakeFromAxisAndAngle(nsVec3(0.0f, 0.0f, 1.0f), nsAngle::MakeFromDegree(90.0f));

    for (nsUInt32 i = 0; i < 2; ++i)
    {
      NS_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset, 0);
      NS_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      NS_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), nsVec3(1.5f, 1.5f, 1.5f), 0);
    }

    for (nsUInt32 i = 2; i < 4; ++i)
    {
      NS_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset + nsVec3(0.0f, 150.0f, 0.0f), eps * 2.0f);
      NS_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q * q, eps * 10.0f));
      NS_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), nsVec3(2.25f, 2.25f, 2.25f), 0);
    }
  }

  void SanityCheckWorld(nsWorld& ref_world)
  {
    struct Traverser
    {
      Traverser(nsWorld& ref_world)
        : m_World(ref_world)
      {
      }

      nsWorld& m_World;
      nsSet<nsGameObject*> m_Found;

      nsVisitorExecution::Enum Visit(nsGameObject* pObject)
      {
        nsGameObject* pObject2 = nullptr;
        NS_TEST_BOOL_MSG(m_World.TryGetObject(pObject->GetHandle(), pObject2), "Visited object that is not part of the world!");
        NS_TEST_BOOL_MSG(pObject2 == pObject, "Handle did not resolve to the same object!");
        NS_TEST_BOOL_MSG(!m_Found.Contains(pObject), "Object visited twice!");
        m_Found.Insert(pObject);

        const nsUInt32 uiChildren = pObject->GetChildCount();
        nsUInt32 uiChildren2 = 0;
        for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
        {
          uiChildren2++;
          auto handle = it->GetHandle();
          nsGameObject* pChild = nullptr;
          NS_TEST_BOOL_MSG(m_World.TryGetObject(handle, pChild), "Could not resolve child!");
          nsGameObject* pParent = pChild->GetParent();
          NS_TEST_BOOL_MSG(pParent == pObject, "pObject's child's parent does not point to pObject!");
        }
        NS_TEST_INT(uiChildren, uiChildren2);
        return nsVisitorExecution::Continue;
      }
    };

    Traverser traverser(ref_world);
    ref_world.Traverse(nsWorld::VisitorFunc(&Traverser::Visit, &traverser), nsWorld::TraversalMethod::BreadthFirst);
  }

  class CustomCoordinateSystemProvider : public nsCoordinateSystemProvider
  {
  public:
    CustomCoordinateSystemProvider(const nsWorld* pWorld)
      : nsCoordinateSystemProvider(pWorld)
    {
    }

    virtual void GetCoordinateSystem(const nsVec3& vGlobalPosition, nsCoordinateSystem& out_coordinateSystem) const override
    {
      const nsMat3 mTmp = nsGraphicsUtils::CreateLookAtViewMatrix(-vGlobalPosition, nsVec3(0, 0, 1), nsHandedness::LeftHanded);

      out_coordinateSystem.m_vRightDir = mTmp.GetRow(0);
      out_coordinateSystem.m_vUpDir = mTmp.GetRow(1);
      out_coordinateSystem.m_vForwardDir = mTmp.GetRow(2);
    }
  };

  class VelocityTestModule : public nsWorldModule
  {
    NS_ADD_DYNAMIC_REFLECTION(VelocityTestModule, nsWorldModule);
    NS_DECLARE_WORLD_MODULE();

  public:
    VelocityTestModule(nsWorld* pWorld)
      : nsWorldModule(pWorld)
    {
    }

    virtual void Initialize() override
    {
      {
        auto desc = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::SetLocalPos, this);
        RegisterUpdateFunction(desc);
      }

      {
        auto desc = NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::ResetGlobalPos, this);
        RegisterUpdateFunction(desc);
      }
    }

    void SetLocalPos(const UpdateContext&)
    {
      if (m_bSetLocalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        nsUInt32 i = it->GetHandle().GetInternalID().m_InstanceIndex;

        nsVec3 newPos = nsVec3(i * 10.0f, 0, 0);
        it->SetLocalPosition(newPos);

        nsQuat newRot = nsQuat::MakeFromAxisAndAngle(nsVec3::MakeAxisZ(), nsAngle::MakeFromDegree(i * 30.0f));
        it->SetLocalRotation(newRot);

        if (i > 5)
        {
          it->UpdateGlobalTransform();
        }
        if (i > 8)
        {
          it->UpdateGlobalTransformAndBounds();
        }
      }
    }

    void ResetGlobalPos(const UpdateContext&)
    {
      if (m_bResetGlobalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SetGlobalPosition(nsVec3::MakeZero());
      }
    }

    bool m_bSetLocalPos = false;
    bool m_bResetGlobalPos = false;
  };

  // clang-format off
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(VelocityTestModule, 1, nsRTTINoAllocator)
  NS_END_DYNAMIC_REFLECTED_TYPE;
  NS_IMPLEMENT_WORLD_MODULE(VelocityTestModule);
  // clang-format on

  nsGameObject* CreateObj(nsWorld* pWorld, nsStringView sName, nsGameObject* pParent = nullptr, nsStringView sGlobalkey = {})
  {
    nsGameObjectDesc gd;
    gd.m_sName.Assign(sName);
    gd.m_hParent = pParent ? pParent->GetHandle() : nsGameObjectHandle();

    nsGameObject* go;
    pWorld->CreateObject(gd, go);

    if (!sGlobalkey.IsEmpty())
    {
      go->SetGlobalKey(sGlobalkey);
    }

    return go;
  }
} // namespace

class nsGameObjectTest
{
public:
  static void TestInternals(nsGameObject* pObject, nsGameObject* pParent, nsUInt32 uiHierarchyLevel)
  {
    NS_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    NS_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);

    if (pParent)
    {
      NS_TEST_BOOL(pObject->m_pTransformationData->m_pParentData->m_pObject == pParent);
    }

    NS_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    NS_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

NS_CREATE_SIMPLE_TEST(World, World)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Transforms dynamic")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    nsVec3 offset = nsVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    world.Update();

    TestTransforms(o, offset);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Transforms static")
  {
    nsWorldDesc worldDesc("Test");
    worldDesc.m_bReportErrorWhenStaticObjectMoves = false;

    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    nsVec3 offset = nsVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    // No need to call world update since global transform is updated immediately for static objects.
    // world.Update();

    TestTransforms(o, offset);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GameObject parenting")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    const float eps = nsMath::DefaultEpsilon<float>();
    nsQuat q = nsQuat::MakeFromAxisAndAngle(nsVec3(0.0f, 0.0f, 1.0f), nsAngle::MakeFromDegree(90.0f));

    nsGameObjectDesc desc;
    desc.m_LocalPosition = nsVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = nsVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    nsGameObject* pParentObject;
    nsGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    NS_TEST_VEC3(pParentObject->GetLocalPosition(), desc.m_LocalPosition, 0);
    NS_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    NS_TEST_VEC3(pParentObject->GetLocalScaling(), desc.m_LocalScaling, 0);

    NS_TEST_VEC3(pParentObject->GetGlobalPosition(), desc.m_LocalPosition, 0);
    NS_TEST_BOOL(pParentObject->GetGlobalRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    NS_TEST_VEC3(pParentObject->GetGlobalScaling(), desc.m_LocalScaling, 0);

    NS_TEST_BOOL(pParentObject->GetName() == desc.m_sName.GetString());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_hParent = parentObject;

    nsGameObjectHandle childObjects[10];
    for (nsUInt32 i = 0; i < 10; ++i)
    {
      nsStringBuilder sb;
      sb.AppendFormat("Child_{0}", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = nsVec3(i * 10.0f, 0.0f, 0.0f);

      childObjects[i] = world.CreateObject(desc);
    }

    nsUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      nsStringBuilder sb;
      sb.AppendFormat("Child_{0}", uiCounter);

      NS_TEST_BOOL(it->GetName() == sb);

      NS_TEST_VEC3(it->GetGlobalPosition(), nsVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      NS_TEST_BOOL(it->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      NS_TEST_VEC3(it->GetGlobalScaling(), nsVec3(1.5f, 1.5f, 1.5f), 0.0f);

      ++uiCounter;
    }

    NS_TEST_INT(uiCounter, 10);
    NS_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObjectNow(childObjects[0]);
    world.DeleteObjectNow(childObjects[3]);
    world.DeleteObjectNow(childObjects[9]);

    NS_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    NS_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    NS_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    nsUInt32 indices[7] = {1, 2, 4, 5, 6, 7, 8};

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      nsStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      NS_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    NS_TEST_INT(uiCounter, 7);
    NS_TEST_INT(pParentObject->GetChildCount(), 7);

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    NS_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    NS_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    NS_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      nsStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      NS_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    NS_TEST_INT(uiCounter, 7);
    NS_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObjectDelayed(parentObject);
    NS_TEST_BOOL(world.IsValidObject(parentObject));

    for (nsUInt32 i = 0; i < NS_ARRAY_SIZE(indices); ++i)
    {
      NS_TEST_BOOL(world.IsValidObject(childObjects[indices[i]]));
    }

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    NS_TEST_BOOL(!world.IsValidObject(parentObject));

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    NS_TEST_INT(world.GetObjectCount(), 0);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Re-parenting 1")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    nsGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    nsGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1);
    nsGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    nsGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2);

    NS_TEST_INT(o.pParent1->GetChildCount(), 2);
    auto it = o.pParent1->GetChildren();
    NS_TEST_BOOL(o.pChild11 == it);
    ++it;
    NS_TEST_BOOL(o.pParent2 == it);
    ++it;
    NS_TEST_BOOL(!it.IsValid());

    it = o.pParent2->GetChildren();
    NS_TEST_BOOL(o.pChild21 == it);
    ++it;
    NS_TEST_BOOL(!it.IsValid());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Re-parenting 2")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pChild21->SetParent(nsGameObjectHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    nsGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    nsGameObjectTest::TestInternals(o.pParent2, nullptr, 0);
    nsGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    nsGameObjectTest::TestInternals(o.pChild21, nullptr, 0);

    auto it = o.pParent1->GetChildren();
    NS_TEST_BOOL(o.pChild11 == it);
    ++it;
    NS_TEST_BOOL(!it.IsValid());

    NS_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    NS_TEST_BOOL(!it.IsValid());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Re-parenting 3")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);
    SanityCheckWorld(world);
    // Here we test whether the sibling information is correctly cleared.

    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling.
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) and next (pParent2) sibling.
    o.pChild21->SetParent(nsGameObjectHandle());
    SanityCheckWorld(world);
    // pChild21 has no siblings.
    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling again.
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Traversal")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        nsVisitorExecution::Enum Visit(nsGameObject* pObject)
        {
          if (m_uiCounter < NS_ARRAY_SIZE(m_o.pObjects))
          {
            NS_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return nsVisitorExecution::Continue;
        }

        nsUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      BreadthFirstTest bft;
      bft.m_o = o;

      world.Traverse(nsWorld::VisitorFunc(&BreadthFirstTest::Visit, &bft), nsWorld::BreadthFirst);
      NS_TEST_INT(bft.m_uiCounter, NS_ARRAY_SIZE(o.pObjects));
    }

    {
      world.CreateObject(nsGameObjectDesc());

      struct DepthFirstTest
      {
        DepthFirstTest() { m_uiCounter = 0; }

        nsVisitorExecution::Enum Visit(nsGameObject* pObject)
        {
          if (m_uiCounter == 0)
          {
            NS_TEST_BOOL(pObject == m_o.pParent1);
          }
          else if (m_uiCounter == 1)
          {
            NS_TEST_BOOL(pObject == m_o.pChild11);
          }
          else if (m_uiCounter == 2)
          {
            NS_TEST_BOOL(pObject == m_o.pParent2);
          }
          else if (m_uiCounter == 3)
          {
            NS_TEST_BOOL(pObject == m_o.pChild21);
          }

          ++m_uiCounter;
          if (m_uiCounter >= NS_ARRAY_SIZE(m_o.pObjects))
            return nsVisitorExecution::Stop;

          return nsVisitorExecution::Continue;
        }

        nsUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      DepthFirstTest dft;
      dft.m_o = o;

      world.Traverse(nsWorld::VisitorFunc(&DepthFirstTest::Visit, &dft), nsWorld::DepthFirst);
      NS_TEST_INT(dft.m_uiCounter, NS_ARRAY_SIZE(o.pObjects));
    }

    {
      NS_TEST_INT(world.GetObjectCount(), 5);
      world.DeleteObjectNow(o.pChild11->GetHandle(), false);
      NS_TEST_INT(world.GetObjectCount(), 4);

      for (auto it = world.GetObjects(); it.IsValid(); ++it)
      {
        NS_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }

      const nsWorld& constWorld = world;
      for (auto it = constWorld.GetObjects(); it.IsValid(); ++it)
      {
        NS_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Multiple Worlds")
  {
    nsWorldDesc worldDesc1("Test1");
    nsWorld world1(worldDesc1);
    NS_LOCK(world1.GetWriteMarker());

    nsWorldDesc worldDesc2("Test2");
    nsWorld world2(worldDesc2);
    NS_LOCK(world2.GetWriteMarker());

    nsGameObjectDesc desc;
    desc.m_sName.Assign("Obj1");

    nsGameObjectHandle hObj1 = world1.CreateObject(desc);
    NS_TEST_BOOL(world1.IsValidObject(hObj1));

    desc.m_sName.Assign("Obj2");

    nsGameObjectHandle hObj2 = world2.CreateObject(desc);
    NS_TEST_BOOL(world2.IsValidObject(hObj2));

    nsGameObject* pObj1 = nullptr;
    const nsGameObject* pObjConst = nullptr;
    NS_TEST_BOOL(world1.TryGetObject(hObj1, pObj1));
    NS_TEST_BOOL(pObj1 != nullptr);

    pObj1->SetGlobalKey("Obj1");
    pObj1 = nullptr;
    NS_TEST_BOOL(world1.TryGetObjectWithGlobalKey(nsTempHashedString("Obj1"), pObj1));
    NS_TEST_BOOL(!world1.TryGetObjectWithGlobalKey(nsTempHashedString("Obj2"), pObj1));
    NS_TEST_BOOL(pObj1 != nullptr);
    NS_TEST_BOOL(world1.TryGetObjectWithGlobalKey(nsTempHashedString("Obj1"), pObjConst));
    NS_TEST_BOOL(pObj1 == pObjConst);

    nsGameObject* pObj2 = nullptr;
    NS_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    NS_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Obj2");
    pObj2 = nullptr;
    NS_TEST_BOOL(world2.TryGetObjectWithGlobalKey(nsTempHashedString("Obj2"), pObj2));
    NS_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(nsTempHashedString("Obj1"), pObj2));
    NS_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Deschd");
    NS_TEST_BOOL(world2.TryGetObjectWithGlobalKey(nsTempHashedString("Deschd"), pObj2));
    NS_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(nsTempHashedString("Obj2"), pObj2));

    world2.DeleteObjectNow(hObj2);

    NS_TEST_BOOL(!world2.IsValidObject(hObj2));
    NS_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(nsTempHashedString("Deschd"), pObj2));
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Custom coordinate system")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);

    nsSharedPtr<CustomCoordinateSystemProvider> pProvider = NS_DEFAULT_NEW(CustomCoordinateSystemProvider, &world);
    CustomCoordinateSystemProvider* pProviderBackup = pProvider.Borrow();

    world.SetCoordinateSystemProvider(pProvider);
    NS_TEST_BOOL(&world.GetCoordinateSystemProvider() == pProviderBackup);

    nsVec3 pos = nsVec3(2, 3, 0);

    nsCoordinateSystem coordSys;
    world.GetCoordinateSystem(pos, coordSys);

    NS_TEST_VEC3(coordSys.m_vForwardDir, (-pos).GetNormalized(), nsMath::SmallEpsilon<float>());
    NS_TEST_VEC3(coordSys.m_vUpDir, nsVec3(0, 0, 1), nsMath::SmallEpsilon<float>());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Active Flag / Active State")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    nsGameObjectHandle hParent;
    nsGameObjectDesc desc;
    nsGameObjectHandle hObjects[10];
    nsGameObject* pObjects[10];

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      desc.m_hParent = hParent;
      hObjects[i] = world.CreateObject(desc, pObjects[i]);
      hParent = hObjects[i];

      NS_TEST_BOOL(pObjects[i]->GetActiveFlag());
      NS_TEST_BOOL(pObjects[i]->IsActive());
    }

    nsUInt32 iTopDisabled = 1;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      NS_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(pObjects[i]->GetActiveFlag() == true);
      NS_TEST_BOOL(pObjects[i]->IsActive() == true);
    }

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      NS_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    iTopDisabled = 3;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (nsUInt32 i = 0; i < 10; ++i)
    {
      NS_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }
  }

#if NS_ENABLED(NS_GAMEOBJECT_VELOCITY)
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Velocity")
  {
    constexpr nsUInt32 numObjects = 10;

    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    auto pModule = world.GetOrCreateModule<VelocityTestModule>();

    nsGameObjectDesc objectDesc;
    objectDesc.m_bDynamic = true;

    nsGameObjectHandle hObjects[numObjects];
    nsGameObject* pObjects[numObjects];
    for (nsUInt32 i = 0; i < numObjects; ++i)
    {
      objectDesc.m_LocalPosition = nsVec3(0, 0, 5);
      objectDesc.m_LocalRotation = nsQuat::MakeFromAxisAndAngle(nsVec3::MakeAxisZ(), nsAngle::MakeFromDegree(90));

      hObjects[i] = world.CreateObject(objectDesc, pObjects[i]);
    }

    pModule->m_bSetLocalPos = true;
    pModule->m_bResetGlobalPos = false;

    world.GetClock().SetFixedTimeStep(nsTime::MakeFromMilliseconds(100));
    world.Update();

    for (auto& pObject : pObjects)
    {
      nsUInt32 i = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      nsVec3 expectedLastPos = nsVec3(0, 0, 5);
      nsVec3 expectedPos = nsVec3(i * 10.0f, 0, 0);
      nsVec3 expectedLinearVelocity = nsVec3(i * 100.0f, 0, -50);
      NS_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, nsMath::DefaultEpsilon<float>());
      NS_TEST_VEC3(pObject->GetGlobalPosition(), expectedPos, nsMath::DefaultEpsilon<float>());
      NS_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, nsMath::DefaultEpsilon<float>());

      nsVec3 expectedAngularVelocity = nsVec3(0, 0, (nsAngle::MakeFromDegree(i * 30.0f) - nsAngle::MakeFromDegree(90)).GetRadian() * 10);
      nsVec3 angularVelocity = pObject->GetAngularVelocity();
      NS_TEST_VEC3(angularVelocity, expectedAngularVelocity, nsMath::DefaultEpsilon<float>());
    }

    pModule->m_bSetLocalPos = false;
    pModule->m_bResetGlobalPos = true;

    world.Update();

    for (auto& pObject : pObjects)
    {
      nsUInt32 i = pObject->GetHandle().GetInternalID().m_InstanceIndex;
      nsVec3 expectedLastPos = nsVec3(i * 10.0f, 0, 0);
      nsVec3 expectedLinearVelocity = nsVec3(i * -100.0f, 0, 0);
      NS_TEST_VEC3(pObject->GetLastGlobalTransform().m_vPosition, expectedLastPos, nsMath::DefaultEpsilon<float>());
      NS_TEST_VEC3(pObject->GetGlobalPosition(), nsVec3::MakeZero(), nsMath::DefaultEpsilon<float>());
      NS_TEST_VEC3(pObject->GetLinearVelocity(), expectedLinearVelocity, nsMath::DefaultEpsilon<float>());
    }
  }
#endif

  NS_TEST_BLOCK(nsTestBlock::Enabled, "FindChildByName / FindChildByPath / SearchForChildByNameSequence")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    auto pRoot1 = CreateObj(&world, "Root1");
    auto pRoot2 = CreateObj(&world, "Root2");

    auto pA1 = CreateObj(&world, "A1", pRoot1);
    auto pB1 = CreateObj(&world, "B1", pRoot1);
    auto pC1 = CreateObj(&world, "C1", pRoot1);

    auto pA2 = CreateObj(&world, "A2", pA1);
    auto pB2 = CreateObj(&world, "B2", pA1);
    auto pC2 = CreateObj(&world, "C2", pA1);

    nsCollectionComponent* pCol;
    nsCollectionComponent::CreateComponent(pC2, pCol);

    // FindChildByName

    NS_TEST_BOOL(pRoot1->FindChildByName("A1", false) == pA1);
    NS_TEST_BOOL(pRoot1->FindChildByName("B1", false) == pB1);

    NS_TEST_BOOL(pRoot2->FindChildByName("A1", false) == nullptr);

    NS_TEST_BOOL(pRoot1->FindChildByName("A2", false) == nullptr);
    NS_TEST_BOOL(pRoot1->FindChildByName("B2", false) == nullptr);

    NS_TEST_BOOL(pRoot1->FindChildByName("A2", true) == pA2);
    NS_TEST_BOOL(pRoot1->FindChildByName("B2", true) == pB2);

    // FindChildByPath

    NS_TEST_BOOL(pRoot1->FindChildByPath("") == pRoot1);
    NS_TEST_BOOL(pA1->FindChildByPath("") == pA1);

    NS_TEST_BOOL(pRoot1->FindChildByPath("A1") == pA1);
    NS_TEST_BOOL(pRoot1->FindChildByPath("B1") == pB1);

    NS_TEST_BOOL(pRoot1->FindChildByPath("A2") == nullptr);
    NS_TEST_BOOL(pRoot1->FindChildByPath("B2") == nullptr);

    NS_TEST_BOOL(pRoot1->FindChildByPath("A1/A2") == pA2);
    NS_TEST_BOOL(pRoot1->FindChildByPath("A1/B2") == pB2);

    // SearchForChildByNameSequence

    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("") == pRoot1);
    NS_TEST_BOOL(pA1->SearchForChildByNameSequence("") == pA1);

    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1") == pA1);
    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("B1") == pB1);

    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A2") == pA2);
    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("B2") == pB2);

    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/A2") == pA2);
    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/B2") == pB2);

    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("A1/B2", nsGetStaticRTTI<nsComponent>()) == nullptr);
    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("", nsGetStaticRTTI<nsCollectionComponent>()) == nullptr);
    NS_TEST_BOOL(pRoot1->SearchForChildByNameSequence("C2", nsGetStaticRTTI<nsCollectionComponent>()) == pC2);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SearchForObject")
  {
    nsWorldDesc worldDesc("Test");
    nsWorld world(worldDesc);
    NS_LOCK(world.GetWriteMarker());

    auto pKey1 = CreateObj(&world, "Key1", nullptr, "Key1");
    auto pKey2 = CreateObj(&world, "Key2", nullptr, "Key2");
    auto pKey3 = CreateObj(&world, "Key3", nullptr, "Key3");

    auto pA1 = CreateObj(&world, "A", pKey1);
    auto pB1 = CreateObj(&world, "B", pA1);
    auto pC1 = CreateObj(&world, "C", pB1);

    auto pA2 = CreateObj(&world, "A", pKey2, "A2");
    auto pB2 = CreateObj(&world, "B", pA2, "B2");
    auto pC2 = CreateObj(&world, "C", pB2, "C2");

    nsCollectionComponent* pCol;
    nsCollectionComponent::CreateComponent(pC2, pCol);

    NS_TEST_BOOL(world.SearchForObject("G:Key1") == pKey1);
    NS_TEST_BOOL(world.SearchForObject("G:Key2") == pKey2);
    NS_TEST_BOOL(world.SearchForObject("G:Key3/") == pKey3);
    NS_TEST_BOOL(world.SearchForObject("G:key3/") == nullptr); // case sensitive
    NS_TEST_BOOL(world.SearchForObject("G:B2") == pB2);
    NS_TEST_BOOL(world.SearchForObject("G:none") == nullptr);

    NS_TEST_BOOL(world.SearchForObject("A", pKey1) == pA1);
    NS_TEST_BOOL(world.SearchForObject("B", pKey1) == pB1);
    NS_TEST_BOOL(world.SearchForObject("A/B", pKey1) == pB1);
    NS_TEST_BOOL(world.SearchForObject("A/C", pKey1) == pC1);
    NS_TEST_BOOL(world.SearchForObject("B/C", pA1) == pC1);
    NS_TEST_BOOL(world.SearchForObject("B/C", pA1, nsGetStaticRTTI<nsCollectionComponent>()) == nullptr);
    NS_TEST_BOOL(world.SearchForObject("A", pA1) == nullptr); // A has to be a child

    NS_TEST_BOOL(world.SearchForObject("G:A2/C") == pC2);

    NS_TEST_BOOL(world.SearchForObject("P:A", pC2) == pA2);
    NS_TEST_BOOL(world.SearchForObject("P:D", pC2) == nullptr);
    NS_TEST_BOOL(world.SearchForObject("P:A/C", pC2) == pC2);
    NS_TEST_BOOL(world.SearchForObject("P:A/C", pC2, nsGetStaticRTTI<nsCollectionComponent>()) == pC2);

    NS_TEST_BOOL(world.SearchForObject("", pA1) == pA1);
    NS_TEST_BOOL(world.SearchForObject("") == nullptr);

    NS_TEST_BOOL(world.SearchForObject("G:C2/P:A/B", pKey3) == pB2);
    NS_TEST_BOOL(world.SearchForObject("G:C2/P:a/B", pKey3) == nullptr);  // case sensitive
    NS_TEST_BOOL(world.SearchForObject("G:C2/P:A//B", pKey3) == nullptr); // malformed path

    NS_TEST_BOOL(world.SearchForObject("..", pC1) == pB1);
    NS_TEST_BOOL(world.SearchForObject("../../", pC1) == pA1);
    NS_TEST_BOOL(world.SearchForObject("../..", pC1) == pA1);

    NS_TEST_BOOL(world.SearchForObject("G:C2/P:B/../../A", pKey3) == pA2);

    NS_TEST_BOOL(world.SearchForObject("G:B2/C/..") == nullptr); // malformed path
  }
}
