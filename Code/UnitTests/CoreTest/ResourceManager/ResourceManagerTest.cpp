#include <CoreTest/CoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/ScopeExit.h>

NS_CREATE_SIMPLE_TEST_GROUP(ResourceManager);

namespace
{
  using TestResourceHandle = nsTypedResourceHandle<class TestResource>;

  struct TestResourceDescriptor
  {
    nsUInt32 m_uiData = 0;
  };

  class TestResource : public nsResource
  {
    NS_ADD_DYNAMIC_REFLECTION(TestResource, nsResource);
    NS_RESOURCE_DECLARE_COMMON_CODE(TestResource);
    NS_RESOURCE_DECLARE_CREATEABLE(TestResource, TestResourceDescriptor);

  public:
    TestResource()
      : nsResource(nsResource::DoUpdate::OnAnyThread, 1)
    {
    }

  protected:
    virtual nsResourceLoadDesc UnloadData(Unload WhatToUnload) override
    {
      nsResourceLoadDesc ld;
      ld.m_State = nsResourceState::Unloaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      return ld;
    }

    virtual nsResourceLoadDesc UpdateContent(nsStreamReader* Stream) override
    {
      nsResourceLoadDesc ld;
      ld.m_State = nsResourceState::Loaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      nsStreamReader& s = *Stream;

      nsUInt32 uiNumElements = 0;
      s >> uiNumElements;

      if (GetResourceID().StartsWith("NonBlockingLevel1-"))
      {
        m_hNested = nsResourceManager::LoadResource<TestResource>("Level0-0");
      }

      if (GetResourceID().StartsWith("BlockingLevel1-"))
      {
        m_hNested = nsResourceManager::LoadResource<TestResource>("Level0-0");

        nsResourceLock<TestResource> pTestResource(m_hNested, nsResourceAcquireMode::BlockTillLoaded_NeverFail);

        NS_ASSERT_ALWAYS(pTestResource.GetAcquireResult() == nsResourceAcquireResult::Final, "");
      }

      m_Data.SetCountUninitialized(uiNumElements);

      for (nsUInt32 i = 0; i < uiNumElements; ++i)
      {
        s >> m_Data[i];
      }

      return ld;
    }

    virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override
    {
      out_NewMemoryUsage.m_uiMemoryCPU = sizeof(TestResource);
      out_NewMemoryUsage.m_uiMemoryGPU = 0;
    }

  public:
    void Test() { NS_TEST_BOOL(!m_Data.IsEmpty()); }

  private:
    TestResourceHandle m_hNested;
    nsDynamicArray<nsUInt32> m_Data;
  };

  NS_RESOURCE_IMPLEMENT_CREATEABLE(TestResource, TestResourceDescriptor)
  {
    nsResourceLoadDesc res;
    res.m_State = nsResourceState::Loaded;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;

    NS_TEST_INT(m_Data.GetCount(), 0);
    m_Data.PushBack(descriptor.m_uiData);
    nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(50));
    NS_TEST_INT(m_Data.GetCount(), 1);

    return res;
  }

  class TestResourceTypeLoader : public nsResourceTypeLoader
  {
  public:
    struct LoadedData
    {
      nsDefaultMemoryStreamStorage m_StreamData;
      nsMemoryStreamReader m_Reader;
    };

    virtual nsResourceLoadData OpenDataStream(const nsResource* pResource) override
    {
      LoadedData* pData = NS_DEFAULT_NEW(LoadedData);

      const nsUInt32 uiNumElements = 1024 * 10;
      pData->m_StreamData.Reserve(uiNumElements * sizeof(nsUInt32) + 1);

      nsMemoryStreamWriter writer(&pData->m_StreamData);
      pData->m_Reader.SetStorage(&pData->m_StreamData);

      writer << uiNumElements;

      for (nsUInt32 i = 0; i < uiNumElements; ++i)
      {
        writer << i;
      }

      nsResourceLoadData ld;
      ld.m_pCustomLoaderData = pData;
      ld.m_pDataStream = &pData->m_Reader;
      ld.m_sResourceDescription = pResource->GetResourceID();

      return ld;
    }

    virtual void CloseDataStream(const nsResource* pResource, const nsResourceLoadData& loaderData) override
    {
      LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      NS_DEFAULT_DELETE(pData);
    }
  };

  NS_RESOURCE_IMPLEMENT_COMMON_CODE(TestResource);
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(TestResource, 1, nsRTTIDefaultAllocator<TestResource>)
  NS_END_DYNAMIC_REFLECTED_TYPE;


  class ResourceTestThread : public nsThread
  {
  public:
    nsDelegate<int()> m_Lambda;

    ResourceTestThread()
      : nsThread("Resource Test Thread")
    {
    }

    virtual nsUInt32 Run()
    {
      if (m_Lambda.IsValid())
      {
        return m_Lambda();
      }
      return 0;
    }
  };
} // namespace

NS_CREATE_SIMPLE_TEST(ResourceManager, Basics)
{
  TestResourceTypeLoader TypeLoader;
  nsResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  nsResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  NS_SCOPE_EXIT(nsResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Main")
  {
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const nsUInt32 uiNumResources = 200;

    nsDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    nsStringBuilder sResourceID;
    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("Level0-{}", i);
      hResources.PushBack(nsResourceManager::LoadResource<TestResource>(sResourceID));
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceManager::PreloadResource(hResources[i]);
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceLock<TestResource> pTestResource(hResources[i], nsResourceAcquireMode::BlockTillLoaded_NeverFail);

      NS_TEST_BOOL(pTestResource.GetAcquireResult() == nsResourceAcquireResult::Final);

      pTestResource->Test();
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    hResources.Clear();

    nsUInt32 uiUnloaded = 0;

    for (nsUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += nsResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == uiNumResources)
        break;

      nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(100));
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}


NS_CREATE_SIMPLE_TEST(ResourceManager, ThreadedCreateResource)
{
  TestResourceTypeLoader TypeLoader;
  nsResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  nsResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  NS_SCOPE_EXIT(nsResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  auto testLambda = []() -> int
  {
    TestResourceDescriptor desc;
    TestResourceHandle hTest = nsResourceManager::GetOrCreateResource<TestResource>("test1", std::move(desc));
    {
      nsResourceLock<TestResource> pTest(hTest, nsResourceAcquireMode::PointerOnly);
      NS_TEST_INT((int)pTest.GetAcquireResult(), (int)nsResourceAcquireResult::Final);
    }
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);
    return 0;
  };

  NS_TEST_BLOCK(nsTestBlock::Enabled, "ThreadedCreateResource")
  {
    nsHybridArray<nsUniquePtr<ResourceTestThread>, 8> threads;
    for (nsUInt32 i = 0; i < 8; ++i)
    {
      nsUniquePtr<ResourceTestThread> thread = NS_DEFAULT_NEW(ResourceTestThread);
      thread->m_Lambda = testLambda;
      threads.PushBack(std::move(thread));
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    for (nsUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Start();
    }

    for (nsUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Join();
    }
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);

    threads.Clear();
  }
}


NS_CREATE_SIMPLE_TEST(ResourceManager, NestedLoading)
{
  TestResourceTypeLoader TypeLoader;
  nsResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  nsResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  NS_SCOPE_EXIT(nsResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  NS_TEST_BLOCK(nsTestBlock::Enabled, "NonBlocking")
  {
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const nsUInt32 uiNumResources = 200;

    nsDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    nsStringBuilder sResourceID;
    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("NonBlockingLevel1-{}", i);
      hResources.PushBack(nsResourceManager::LoadResource<TestResource>(sResourceID));
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceManager::PreloadResource(hResources[i]);
    }

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceLock<TestResource> pTestResource(hResources[i], nsResourceAcquireMode::BlockTillLoaded_NeverFail);

      NS_TEST_BOOL(pTestResource.GetAcquireResult() == nsResourceAcquireResult::Final);

      pTestResource->Test();
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (nsResourceManager::IsAnyLoadingInProgress())
    {
      nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(100));
    }

    nsResourceManager::FreeAllUnusedResources();
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }

  // Test disabled as it deadlocks
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Blocking")
  {
    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const nsUInt32 uiNumResources = 500;

    nsDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    nsStringBuilder sResourceID;
    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("BlockingLevel1-{}", i);
      hResources.PushBack(nsResourceManager::LoadResource<TestResource>(sResourceID));
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceManager::PreloadResource(hResources[i]);
    }

    for (nsUInt32 i = 0; i < uiNumResources; ++i)
    {
      nsResourceLock<TestResource> pTestResource(hResources[i], nsResourceAcquireMode::BlockTillLoaded_NeverFail);

      NS_TEST_BOOL(pTestResource.GetAcquireResult() == nsResourceAcquireResult::Final);

      pTestResource->Test();
    }

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (nsResourceManager::IsAnyLoadingInProgress())
    {
      nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(100));
    }

    nsResourceManager::FreeAllUnusedResources();
    nsThreadUtils::Sleep(nsTime::MakeFromMilliseconds(100));
    nsResourceManager::FreeAllUnusedResources();

    NS_TEST_INT(nsResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}
