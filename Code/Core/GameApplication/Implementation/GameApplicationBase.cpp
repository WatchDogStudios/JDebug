#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

nsGameApplicationBase* nsGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

nsGameApplicationBase::nsGameApplicationBase(nsStringView sAppName)
  : nsApplication(sAppName)
  , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", nsMakeDelegate(&nsGameApplicationBase::TakeScreenshot, this))
  , m_ConFunc_CaptureFrame("CaptureFrame", "()", nsMakeDelegate(&nsGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

nsGameApplicationBase::~nsGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(nsStringBuilder& out_sString)
{
  const nsDateTime dt = nsDateTime::MakeFromTimestamp(nsTimestamp::CurrentTimestamp());

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), nsArgU(dt.GetMonth(), 2, true), nsArgU(dt.GetDay(), 2, true), nsArgU(dt.GetHour(), 2, true), nsArgU(dt.GetMinute(), 2, true), nsArgU(dt.GetSecond(), 2, true), nsArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void nsGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public nsTask
  {
  public:
    nsProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask() = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      nsStringBuilder sPath(":appdata/Profiling/", nsApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      nsFileWriter fileWriter;
      if (fileWriter.Open(sPath) == NS_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        nsLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        nsLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  nsSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = NS_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", nsTaskNesting::Never);
  nsProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  nsTaskSystem::StartSingleTask(pWriteProfilingDataTask, nsTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void nsGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void nsGameApplicationBase::StoreScreenshot(nsImage&& image, nsStringView sContext /*= {} */)
{
  class WriteFileTask final : public nsTask
  {
  public:
    nsImage m_Image;
    nsStringBuilder m_sPath;

    WriteFileTask() = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(nsImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        nsLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  nsSharedPtr<WriteFileTask> pWriteTask = NS_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", nsTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.SetFormat(":appdata/Screenshots/{0}", nsApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(sContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  nsTaskSystem::StartSingleTask(pWriteTask, nsTaskPriority::LongRunning);
}

void nsGameApplicationBase::ExecuteTakeScreenshot(nsWindowOutputTargetBase* pOutputTarget, nsStringView sContext /* = {} */)
{
  if (m_bTakeScreenshot)
  {
    NS_PROFILE_SCOPE("ExecuteTakeScreenshot");
    nsImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(std::move(img), sContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void nsGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void nsGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool nsGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


nsResult nsGameApplicationBase::GetAbsFrameCaptureOutputPath(nsStringBuilder& ref_sOutputPath)
{
  nsStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return nsFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void nsGameApplicationBase::ExecuteFrameCapture(nsWindowHandle targetWindowHandle, nsStringView sContext /*= {} */)
{
  nsFrameCaptureInterface* pCaptureInterface = nsSingletonRegistry::GetSingletonInstance<nsFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  NS_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      nsStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(sContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      nsStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        nsLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        nsLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

void nsGameApplicationBase::ActivateGameState(nsWorld* pWorld, nsStringView sStartPosition, const nsTransform& startPositionOffset)
{
  NS_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState();

  NS_ASSERT_ALWAYS(m_pGameState != nullptr, "Failed to create a game state.");

  m_pGameState->OnActivation(pWorld, sStartPosition, startPositionOffset);

  nsGameApplicationStaticEvent e;
  e.m_Type = nsGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  NS_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());
}

void nsGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  NS_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  nsGameApplicationStaticEvent e;
  e.m_Type = nsGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  nsWindowManager::GetSingleton()->CloseAll(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

nsUniquePtr<nsGameStateBase> nsGameApplicationBase::CreateGameState()
{
  NS_LOG_BLOCK("Create Game State");

  nsUniquePtr<nsGameStateBase> pCurState;

  nsRTTI::ForEachDerivedType<nsGameStateBase>(
    [&](const nsRTTI* pRtti)
    {
      nsUniquePtr<nsGameStateBase> pNewState = pRtti->GetAllocator()->Allocate<nsGameStateBase>();

      if (pCurState == nullptr)

      {
        pCurState = std::move(pNewState);
        return;
      }

      if (pCurState->IsFallbackGameState() && !pNewState->IsFallbackGameState())
      {
        pCurState = std::move(pNewState);
        return;
      }

      if (pCurState->IsFallbackGameState() && pNewState->IsFallbackGameState())
      {
        if (pNewState->GetDynamicRTTI()->IsDerivedFrom(pCurState->GetDynamicRTTI()))
        {
          pCurState = std::move(pNewState);
          return;
        }

        nsLog::Warning("Multiple fallback game states found: '{}' and '{}'", pNewState->GetDynamicRTTI()->GetTypeName(), pCurState->GetDynamicRTTI()->GetTypeName());
        return;
      }

      if (!pCurState->IsFallbackGameState() && !pNewState->IsFallbackGameState())
      {
        nsLog::Warning("Multiple game state implementations found: '{}' and '{}'", pNewState->GetDynamicRTTI()->GetTypeName(), pCurState->GetDynamicRTTI()->GetTypeName());
        return;
      }
    },
    nsRTTI::ForEachOptions::ExcludeNotConcrete);

  return pCurState;
}

void nsGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState(nullptr, {}, nsTransform::MakeIdentity());
}

nsResult nsGameApplicationBase::BeforeCoreSystemsStartup()
{
  nsStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void nsGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (ShouldApplicationQuit())
  {
    return;
  }

  nsStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void nsGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void nsGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    nsResourceManager::EngineAboutToShutdown();
    nsResourceManager::ExecuteAllResourceCleanupCallbacks();
    nsResourceManager::FreeAllUnusedResources();
  }
}

void nsGameApplicationBase::BeforeCoreSystemsShutdown()
{
  if (nsWindowManager::GetSingleton() != nullptr)
  {
    nsWindowManager::GetSingleton()->CloseAll(nullptr);
  }

  {
    nsFrameAllocator::Reset();
    nsResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    nsResourceManager::FreeAllUnusedResources();
  }

  nsTaskSystem::BroadcastClearThreadLocalsEvent();

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    nsTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

NS_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  NS_IGNORE_UNUSED(param0);
  NS_IGNORE_UNUSED(param1);
  NS_IGNORE_UNUSED(param2);
  NS_IGNORE_UNUSED(param3);

  s_bUpdatePluginsExecuted = true;
}

void nsGameApplicationBase::Run()
{
  RunOneFrame();
}

void nsGameApplicationBase::RunOneFrame()
{
  nsProfilingSystem::StartNewFrame();

  NS_PROFILE_SCOPE("Run");
  s_bUpdatePluginsExecuted = false;

  nsWindowManager::GetSingleton()->Update();

  const nsGameUpdateMode state = GetGameUpdateMode();
  if (state == nsGameUpdateMode::Skip)
    return;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    NS_PROFILE_SCOPE("GameApp_BeginAppTick");
    NS_BROADCAST_EVENT(GameApp_BeginAppTick);
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  if (state == nsGameUpdateMode::UpdateInputAndRender)
    Run_InputUpdate();

  Run_AcquireImage();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    NS_ASSERT_DEV(s_bUpdatePluginsExecuted, "nsGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    NS_PROFILE_SCOPE("GameApp_EndAppTick");
    NS_BROADCAST_EVENT(GameApp_EndAppTick);

    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    NS_PROFILE_SCOPE("BeforePresent");
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    NS_PROFILE_SCOPE("Run_PresentImage");
    Run_PresentImage();
  }
  nsClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    NS_PROFILE_SCOPE("AfterPresent");
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    NS_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
}

void nsGameApplicationBase::Run_InputUpdate()
{
  NS_PROFILE_SCOPE("Run_InputUpdate");
  nsInputManager::Update(nsClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool nsGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void nsGameApplicationBase::Run_AcquireImage()
{
}

void nsGameApplicationBase::Run_BeforeWorldUpdate()
{
  NS_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void nsGameApplicationBase::Run_AfterWorldUpdate()
{
  NS_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();

    m_pGameState->ConfigureMainCamera();
  }

  {
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void nsGameApplicationBase::Run_UpdatePlugins()
{
  NS_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  NS_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    nsGameApplicationExecutionEvent e;
    e.m_Type = nsGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void nsGameApplicationBase::Run_PresentImage() {}

void nsGameApplicationBase::Run_FinishFrame()
{
  nsTelemetry::PerFrameUpdate();
  nsResourceManager::PerFrameUpdate();
  nsTaskSystem::FinishFrameTasks();
  nsFrameAllocator::Swap();

#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
  // if many messages have been logged, make sure they get written to disk
  nsLog::Flush(100, nsTime::MakeFromSeconds(10));
#endif

  // reset this state
  m_bTakeScreenshot = false;
}

void nsGameApplicationBase::UpdateFrameTime()
{
  // Do not use nsClock for this, it smooths and clamps the timestep
  const nsTime tNow = nsClock::GetGlobalClock()->GetLastUpdateTime();

  static nsTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}



NS_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
