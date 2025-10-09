#include <CoreTest/CoreTestPCH.h>

#include <Core/Utils/IntervalScheduler.h>

NS_CREATE_SIMPLE_TEST_GROUP(Utils);

namespace
{
  struct TestWork
  {
    float m_IntervalMs = 0.0f;
    nsUInt32 m_Counter = 0;

    void Run()
    {
      ++m_Counter;
    }
  };
} // namespace

NS_CREATE_SIMPLE_TEST(Utils, IntervalScheduler)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "Constant workload")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    nsHybridArray<TestWork, 32> works;
    nsIntervalScheduler<TestWork*> scheduler;

    for (nsUInt32 i = 0; i < NS_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr nsUInt32 uiNumIterations = 60;
    constexpr nsTime timeStep = nsTime::MakeFromMilliseconds(10);

    nsUInt32 wrongDelta = 0;
    for (nsUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, nsTime deltaTime)
        {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = pWork->m_IntervalMs * 0.3;
          const double midValue = pWork->m_IntervalMs + 1.0 - variance;
          if (nsMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      NS_TEST_FLOAT(fNumWorks, 2.5f, 0.5f);

      for (auto& work : works)
      {
        NS_TEST_BOOL(scheduler.GetInterval(&work) == nsTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~120 scheduled works is ok
    NS_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / nsMath::Max(work.m_IntervalMs, 10.0f);

      // check for roughly expected or a little bit more
      NS_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 3.0f, 4.0f);
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Constant workload (bigger delta)")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    nsHybridArray<TestWork, 32> works;
    nsIntervalScheduler<TestWork*> scheduler;

    for (nsUInt32 i = 0; i < NS_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(work.m_IntervalMs));
    }

    constexpr nsUInt32 uiNumIterations = 60;
    constexpr nsTime timeStep = nsTime::MakeFromMilliseconds(20);

    nsUInt32 wrongDelta = 0;
    for (nsUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(timeStep, [&](TestWork* pWork, nsTime deltaTime)
        {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = nsMath::Max(pWork->m_IntervalMs, 20.0f) * 0.3;
          const double midValue = nsMath::Max(pWork->m_IntervalMs, 20.0f) + 1.0 - variance;
          if (nsMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      NS_TEST_FLOAT(fNumWorks, 3.5f, 0.5f);

      for (auto& work : works)
      {
        NS_TEST_BOOL(scheduler.GetInterval(&work) == nsTime::MakeFromMilliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~150 scheduled works is ok
    NS_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = static_cast<float>(uiNumIterations * timeStep.GetMilliseconds()) / nsMath::Max(work.m_IntervalMs, 20.0f);

      // check for roughly expected or a little bit more
      NS_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 2.0f, 3.0f);
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Dynamic workload")
  {
    nsHybridArray<TestWork, 32> works;

    nsIntervalScheduler<TestWork*> scheduler;

    for (nsUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(i));
    }

    for (nsUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(nsTime::MakeFromMilliseconds(10), [&](TestWork* pWork, nsTime deltaTime)
        {
        pWork->Run();
        ++fNumWorks; });

      NS_TEST_FLOAT(fNumWorks, 15.5f, 0.5f);
    }

    for (nsUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(20 + i));
    }

    float fPrevNumWorks = 15.5f;
    for (nsUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0.0f;
      scheduler.Update(nsTime::MakeFromMilliseconds(10), [&](TestWork* pWork, nsTime deltaTime)
        {
        pWork->Run();
        ++fNumWorks; });

      // fNumWork will slowly ramp up until it reaches the new workload of 22 or 23 per update
      NS_TEST_BOOL(fNumWorks + 1.0f >= fPrevNumWorks);
      NS_TEST_BOOL(fNumWorks <= 23.0f);

      fPrevNumWorks = fNumWorks;
    }

    for (nsUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i];
      scheduler.RemoveWork(&work);
    }

    scheduler.Update(nsTime::MakeFromMilliseconds(10), nsIntervalScheduler<TestWork*>::RunWorkCallback());

    for (nsUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      NS_TEST_BOOL(scheduler.GetInterval(&work) == nsTime::MakeFromMilliseconds(20 + i));

      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(100 + i));
    }

    scheduler.Update(nsTime::MakeFromMilliseconds(10), nsIntervalScheduler<TestWork*>::RunWorkCallback());

    for (nsUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      NS_TEST_BOOL(scheduler.GetInterval(&work) == nsTime::MakeFromMilliseconds(100 + i));
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Update/Remove during schedule")
  {
    nsHybridArray<TestWork, 32> works;

    nsIntervalScheduler<TestWork*> scheduler;

    for (nsUInt32 i = 0; i < 32; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = static_cast<float>((i & 1u));

      scheduler.AddOrUpdateWork(&work, nsTime::MakeFromMilliseconds(i));
    }

    nsUInt32 uiNumWorks = 0;
    scheduler.Update(nsTime::MakeFromMilliseconds(33),
      [&](TestWork* pWork, nsTime deltaTime)
      {
        pWork->Run();
        ++uiNumWorks;

        if (pWork->m_IntervalMs == 0.0f)
        {
          scheduler.RemoveWork(pWork);
        }
        else
        {
          scheduler.AddOrUpdateWork(pWork, nsTime::MakeFromMilliseconds(50));
        }
      });

    NS_TEST_INT(uiNumWorks, 32);
    for (nsUInt32 i = 0; i < 32; ++i)
    {
      const nsUInt32 uiExpectedCounter = 1;
      NS_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }

    uiNumWorks = 0;
    scheduler.Update(nsTime::MakeFromMilliseconds(100),
      [&](TestWork* pWork, nsTime deltaTime)
      {
        NS_TEST_FLOAT(pWork->m_IntervalMs, 1.0f, nsMath::DefaultEpsilon<float>());

        pWork->Run();
        ++uiNumWorks;
      });

    NS_TEST_INT(uiNumWorks, 16);
    for (nsUInt32 i = 0; i < 32; ++i)
    {
      const nsUInt32 uiExpectedCounter = 1 + (i & 1);
      NS_TEST_INT(works[i].m_Counter, uiExpectedCounter);
    }
  }
}
