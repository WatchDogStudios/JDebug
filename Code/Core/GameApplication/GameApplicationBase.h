#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/GameState/GameStateBase.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class nsWindowBase;
struct nsWindowCreationDesc;
class nsWorld;

/// Allows custom code to inject logic at specific points during
/// initialization or during shutdown. The events are listed in
/// the order in which they typically happen.
struct nsGameApplicationStaticEvent
{
  enum class Type
  {
    AfterGameStateActivated,
    BeforeGameStateDeactivated
  };

  Type m_Type;
};

/// \brief Events fired during game application execution phases.
///
/// Allows custom code to inject logic at specific update points during each frame.
/// The events are listed in the order in which they typically happen.
struct nsGameApplicationExecutionEvent
{
  enum class Type
  {
    BeginAppTick,        ///< Fired at the beginning of each application tick
    BeforeWorldUpdates,  ///< Fired before world updates are processed
    AfterWorldUpdates,   ///< Fired after world updates are completed
    BeforeUpdatePlugins, ///< Fired before plugin updates are processed
    AfterUpdatePlugins,  ///< Fired after plugin updates are completed
    BeforePresent,       ///< Fired before presenting rendered frames
    AfterPresent,        ///< Fired after presenting rendered frames
    EndAppTick,          ///< Fired at the end of each application tick
  };

  Type m_Type;
};

/// \brief Defines different update modes for the game application.
enum class nsGameUpdateMode
{
  Skip,                 ///< Skip both updating and rendering
  Render,               ///< Only render, don't update input or game logic
  UpdateInputAndRender, ///< Process input, update game logic, and render
};

/// \brief Base class for game applications that provides fundamental game loop and window management.
///
/// Extends nsApplication with game-specific functionality including game state management,
/// window creation, input handling, screenshot capture, and profiling. Serves as the foundation
/// for both standalone games and editor applications.
class NS_CORE_DLL nsGameApplicationBase : public nsApplication
{
public:
  using SUPER = nsApplication;

  nsGameApplicationBase(nsStringView sAppName);
  ~nsGameApplicationBase();

  /// \name Basics
  ///@{

public:
  /// \brief Returns the nsGameApplicationBase singleton
  static nsGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

protected:
  static nsGameApplicationBase* s_pGameApplicationBaseInstance;

  ///@}
  /// \name Capturing Data
  ///@{

public:
  /// \brief Does a profiling capture and writes it to disk at ':appdata'
  void TakeProfilingCapture();

  /// \brief Schedules a screenshot to be taken at the end of the frame.
  ///
  /// After taking a screenshot, StoreScreenshot() is executed, which may decide where to write the result to.
  void TakeScreenshot();

protected:
  /// \brief Called with the result from taking a screenshot. The default implementation writes the image to disk at ':appdata/Screenshots'
  virtual void StoreScreenshot(nsImage&& image, nsStringView sContext = {});

  void ExecuteTakeScreenshot(nsWindowOutputTargetBase* pOutputTarget, nsStringView sContext = {});

  bool m_bTakeScreenshot = false;

  /// expose TakeScreenshot() as a console function
  nsConsoleFunction<void()> m_ConFunc_TakeScreenshot;

  ///@}
  /// \name Frame Captures
  ///@{

public:
  /// \brief Schedules a frame capture if the corresponding plugin is loaded.
  ///
  /// If continuous capture mode is enabled the currently running frame capture is persisted (and not discarded).
  /// Otherwise, the next frame will be captured and persisted.
  void CaptureFrame();

  /// \brief Controls if frame captures are taken continuously (without being persisted) or only on-demand.
  ///
  /// If continuous frame capture is enabled, calling CaptureFrame() will persist the result of the frame capture that is
  /// currently in progress. If continuous frame capture is disabled, CaptureFrame() will capture and persist the next frame.
  /// Note that continuous capture mode comes with a performance cost, but allows the user to decide on-the-fly if the current
  /// frame capture is to be persisted, e.g., when a unit test image comparison fails.
  void SetContinuousFrameCapture(bool bEnable);
  bool GetContinousFrameCapture() const;

  /// \brief Get the absolute base output path for frame captures.
  virtual nsResult GetAbsFrameCaptureOutputPath(nsStringBuilder& ref_sOutputPath);

protected:
  void ExecuteFrameCapture(nsWindowHandle targetWindowHandle, nsStringView sContext = {});

  bool m_bContinuousFrameCapture = false;
  bool m_bCaptureFrame = false;

  /// expose CaptureFrame() as a console function
  nsConsoleFunction<void()> m_ConFunc_CaptureFrame;

  ///@}
  /// \name GameState
  ///@{
public:
  /// \brief Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ///
  /// Broadcasts local event: nsGameApplicationStaticEvent::AfterGameStateActivated
  /// Broadcasts global event: AfterGameStateActivation(nsGameStateBase*)
  void ActivateGameState(nsWorld* pWorld, nsStringView sStartPosition, const nsTransform& startPositionOffset);

  /// \brief Deactivates and destroys the active game state.
  ///
  /// Broadcasts local event: nsGameApplicationStaticEvent::BeforeGameStateDeactivated
  /// Broadcasts global event: BeforeGameStateDeactivation(nsGameStateBase*)
  void DeactivateGameState();

  /// \brief Returns the currently active game state. Could be nullptr.
  nsGameStateBase* GetActiveGameState() const { return m_pGameState.Borrow(); }

protected:
  /// \brief Creates a game state for the application to use.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual nsUniquePtr<nsGameStateBase> CreateGameState();

  /// \brief Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  nsUniquePtr<nsGameStateBase> m_pGameState;

  ///@}
  /// \name Platform Profile
  ///@{
public:
  /// \brief Returns the nsPlatformProfile that has been loaded for this application
  const nsPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }


protected:
  nsPlatformProfile m_PlatformProfile;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual nsResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  /// \brief Returns the target of the 'project' special data directory.
  ///
  /// The return value of this function will be passed into nsFileSystem::SetSpecialDirectory.
  /// Afterwards, any path starting with the special directory marker (">project/") will point
  /// into this directory.
  virtual nsString FindProjectDirectory() const = 0;

  /// \brief Returns the target of the 'base' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'base' data directory. Target defaults to ">sdk/Data/Base".
  virtual nsString GetBaseDataDirectoryPath() const;

  /// \brief Returns the target of the 'project' data directory.
  ///
  /// Path needs to start with a special directory marker (">marker/").
  /// This is passed into the target of the 'project' data directory. Target defaults to ">project/".
  virtual nsString GetProjectDataDirectoryPath() const;

  /// \brief Executes all 'BaseInit_' functions. Typically done very early, before core system startup
  virtual void ExecuteBaseInitFunctions();
  virtual void BaseInit_ConfigureLogging();

  nsEventSubscriptionID m_LogToConsoleID = 0;
  nsEventSubscriptionID m_LogToVsID = 0;

  /// \brief Executes all 'Init_' functions. Typically done after core system startup
  virtual void ExecuteInitFunctions();
  virtual void Init_PlatformProfile_SetPreferred();
  virtual void Init_ConfigureTelemetry();
  virtual void Init_FileSystem_SetSpecialDirs();
  virtual void Init_LoadRequiredPlugins();
  virtual void Init_ConfigureAssetManagement();
  virtual void Init_FileSystem_ConfigureDataDirs();
  virtual void Init_LoadWorldModuleConfig();
  virtual void Init_LoadProjectPlugins();
  virtual void Init_PlatformProfile_LoadForRuntime();
  virtual void Init_ConfigureTags();
  virtual void Init_ConfigureCVars();
  virtual void Init_SetupGraphicsDevice() = 0;
  virtual void Init_SetupDefaultResources();

  nsEvent<const nsGameApplicationStaticEvent&> m_StaticEvents;

  ///@}
  /// \name Application Shutdown
  ///@{
protected:
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Deinit_ShutdownGraphicsDevice() = 0;
  virtual void Deinit_UnloadPlugins();
  virtual void Deinit_ShutdownLogging();

  ///@}
  /// \name Application Execution
  ///@{

public:
  virtual void Run() override;

  void RunOneFrame();

  nsCopyOnBroadcastEvent<const nsGameApplicationExecutionEvent&> m_ExecutionEvents;

  nsTime GetFrameTime() const { return m_FrameTime; }

protected:
  virtual nsGameUpdateMode GetGameUpdateMode() const { return nsGameUpdateMode::UpdateInputAndRender; }

  virtual void Run_InputUpdate();
  virtual bool Run_ProcessApplicationInput();
  /// \brief This function can be used to acquire a new window from a swap-chain or do any other update operations on windows before the multi-threaded rendering and update phase starts.
  virtual void Run_AcquireImage();
  virtual void Run_WorldUpdateAndRender() = 0;
  virtual void Run_BeforeWorldUpdate();
  virtual void Run_AfterWorldUpdate();
  virtual void Run_UpdatePlugins();
  /// \brief This function can be used to present the final image to a window. It is run at the end of the rendering phase. It can also be used to inspect the swap-chain e.g. for screenshot purposes before presenting.
  virtual void Run_PresentImage();
  virtual void Run_FinishFrame();

  void UpdateFrameTime();

  nsTime m_FrameTime;
  ///@}
};
