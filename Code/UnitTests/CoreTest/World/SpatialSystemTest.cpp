#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static nsSpatialData::Category s_SpecialTestCategory = nsSpatialData::RegisterCategory("SpecialTestCategory", nsSpatialData::Flags::None);

  using TestBoundsComponentManager = nsComponentManager<class TestBoundsComponent, nsBlockStorageType::Compact>;

  class TestBoundsComponent : public nsComponent
  {
    NS_DECLARE_COMPONENT_TYPE(TestBoundsComponent, nsComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(nsMsgUpdateLocalBounds& ref_msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      nsBoundingBox bounds = nsBoundingBox::MakeFromCenterAndHalfExtents(nsVec3::MakeZero(), nsVec3(x, y, z));

      nsSpatialData::Category category = m_SpecialCategory;
      if (category == nsInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? nsDefaultSpatialDataCategories::RenderDynamic : nsDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(nsBoundingBoxSphere::MakeFromBox(bounds), category);
    }

    nsSpatialData::Category m_SpecialCategory = nsInvalidSpatialDataCategory;
  };

  // clang-format off
  NS_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1, nsComponentMode::Static)
  {
    NS_BEGIN_MESSAGEHANDLERS
    {
      NS_MESSAGE_HANDLER(nsMsgUpdateLocalBounds, OnUpdateLocalBounds)
    }
    NS_END_MESSAGEHANDLERS;
  }
  NS_END_COMPONENT_TYPE;
  // clang-format on
} // namespace

NS_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  nsWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  nsWorld world(worldDesc);
  NS_LOCK(world.GetWriteMarker());

  auto& rng = world.GetRandomNumberGenerator();

  nsDynamicArray<nsGameObject*> objects;
  objects.Reserve(1000);

  for (nsUInt32 i = 0; i < 1000; ++i)
  {
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    nsGameObjectDesc desc;
    desc.m_bDynamic = (i >= 500);
    desc.m_LocalPosition = nsVec3(x, y, z);

    nsGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    objects.PushBack(pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);
  }

  world.Update();

  nsSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = nsDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  NS_TEST_BLOCK(nsTestBlock::Enabled, "FindObjectsInSphere")
  {
    nsBoundingSphere testSphere = nsBoundingSphere::MakeFromCenterAndRadius(nsVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    nsDynamicArray<nsGameObject*> objectsInSphere;
    nsHashSet<nsGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      nsBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      NS_TEST_BOOL(testSphere.Overlaps(objSphere));
      NS_TEST_BOOL(!uniqueObjects.Insert(pObject));
      NS_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      nsBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        NS_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((nsGameObject*)it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, [&](nsGameObject* pObject)
      {
      objectsInSphere.PushBack(pObject);
      NS_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return nsVisitorExecution::Continue; });

    for (auto pObject : objectsInSphere)
    {
      nsBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      NS_TEST_BOOL(testSphere.Overlaps(objSphere));
      NS_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      nsBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        NS_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((nsGameObject*)it));
      }
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "FindObjectsInBox")
  {
    nsBoundingBox testBox = nsBoundingBox::MakeFromCenterAndHalfExtents(nsVec3(100.0f, 60.0f, 400.0f), nsVec3(3000.0f));

    nsDynamicArray<nsGameObject*> objectsInBox;
    nsHashSet<nsGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      nsBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      NS_TEST_BOOL(testBox.Overlaps(objBox));
      NS_TEST_BOOL(!uniqueObjects.Insert(pObject));
      NS_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      nsBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        NS_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((nsGameObject*)it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, [&](nsGameObject* pObject)
      {
      objectsInBox.PushBack(pObject);
      NS_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return nsVisitorExecution::Continue; });

    for (auto pObject : objectsInBox)
    {
      nsBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      NS_TEST_BOOL(testBox.Overlaps(objSphere));
      NS_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      nsBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        NS_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((nsGameObject*)it));
      }
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "FindVisibleObjects")
  {
    constexpr uint32_t numUpdates = 13;

    // update a few times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    queryParams.m_uiCategoryBitmask = nsDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    nsMat4 lookAt = nsGraphicsUtils::CreateLookAtViewMatrix(nsVec3::MakeZero(), nsVec3::MakeAxisX(), nsVec3::MakeAxisZ());
    nsMat4 projection = nsGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(nsAngle::MakeFromDegree(80.0f), 1.0f, 1.0f, 10000.0f);

    nsFrustum testFrustum = nsFrustum::MakeFromMVP(projection * lookAt);

    nsDynamicArray<const nsGameObject*> visibleObjects;
    nsHashSet<const nsGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {}, nsVisibilityState::Direct);

    NS_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      NS_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      NS_TEST_BOOL(!uniqueObjects.Insert(pObject));
      NS_TEST_BOOL(pObject->IsDynamic());

      nsVisibilityState::Enum visType = pObject->GetVisibilityState();
      NS_TEST_BOOL(visType == nsVisibilityState::Direct);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      nsGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == nsVolumePosition::Outside)
      {
        nsVisibilityState::Enum visType = pObject->GetVisibilityState();
        NS_TEST_BOOL(visType == nsVisibilityState::Invisible);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        nsVec3 pos = it->GetLocalPosition();

        pos.x += (float)rng.DoubleMinMax(-range, range);
        pos.y += (float)rng.DoubleMinMax(-range, range);
        pos.z += (float)rng.DoubleMinMax(-range, range);

        it->SetLocalPosition(pos);
      }
    }

    world.Update();

    // Check that last frame visible doesn't reset entirely after moving
    for (const nsGameObject* pObject : visibleObjects)
    {
      nsVisibilityState::Enum visType = pObject->GetVisibilityState();
      NS_TEST_BOOL(visType == nsVisibilityState::Direct);
    }
  }

  if (false)
  {
    nsStringBuilder outputPath = nsTestFramework::GetInstance()->GetAbsOutputPath();
    NS_TEST_BOOL(nsFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", nsDataDirUsage::AllowWrites) == NS_SUCCESS);

    nsProfilingUtils::SaveProfilingCapture(":output/profiling.json").IgnoreResult();
  }

  // Test multiple categories for spatial data
  NS_TEST_BLOCK(nsTestBlock::Enabled, "MultipleCategories")
  {
    for (nsUInt32 i = 0; i < objects.GetCount(); ++i)
    {
      nsGameObject* pObject = objects[i];

      TestBoundsComponent* pComponent = nullptr;
      TestBoundsComponent::CreateComponent(pObject, pComponent);
      pComponent->m_SpecialCategory = s_SpecialTestCategory;
    }

    world.Update();

    nsDynamicArray<nsGameObjectHandle> allObjects;
    allObjects.Reserve(world.GetObjectCount());

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      allObjects.PushBack(it->GetHandle());
    }

    for (nsUInt32 i = allObjects.GetCount(); i-- > 0;)
    {
      world.DeleteObjectNow(allObjects[i]);
    }

    world.Update();
  }
}
