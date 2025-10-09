#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>

using nsScriptClassResourceHandle = nsTypedResourceHandle<class nsScriptClassResource>;
class nsScriptInstance;

/// World module responsible for script execution and coroutine management.
///
/// Handles the execution of script functions, manages script coroutines,
/// and provides scheduling for script update functions. This module ensures
/// scripts are properly integrated with the world update cycle.
class NS_CORE_DLL nsScriptWorldModule : public nsWorldModule
{
  NS_DECLARE_WORLD_MODULE();
  NS_ADD_DYNAMIC_REFLECTION(nsScriptWorldModule, nsWorldModule);
  NS_DISALLOW_COPY_AND_ASSIGN(nsScriptWorldModule);

public:
  nsScriptWorldModule(nsWorld* pWorld);
  ~nsScriptWorldModule();

  virtual void Initialize() override;
  virtual void WorldClear() override;

  /// Schedules a script function to be called at regular intervals.
  void AddUpdateFunctionToSchedule(const nsAbstractFunctionProperty* pFunction, void* pInstance, nsTime updateInterval, bool bOnlyWhenSimulating);

  /// Removes a previously scheduled script function from the scheduler.
  void RemoveUpdateFunctionToSchedule(const nsAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// Creates a new coroutine of the specified type with the given name.
  ///
  /// Returns an invalid handle if the creationMode prevents creating a new coroutine
  /// and there is already a coroutine running with the same name on the given instance.
  nsScriptCoroutineHandle CreateCoroutine(const nsRTTI* pCoroutineType, nsStringView sName, nsScriptInstance& inout_instance, nsScriptCoroutineCreationMode::Enum creationMode, nsScriptCoroutine*& out_pCoroutine);

  /// Starts the coroutine with the given arguments.
  ///
  /// Calls the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(nsScriptCoroutineHandle hCoroutine, nsArrayPtr<nsVariant> arguments);

  /// Stops and deletes the coroutine.
  ///
  /// Calls the Stop() function and deletes the coroutine on the next update cycle.
  void StopAndDeleteCoroutine(nsScriptCoroutineHandle hCoroutine);

  /// Stops and deletes all coroutines with the given name on the specified instance.
  void StopAndDeleteCoroutine(nsStringView sName, nsScriptInstance* pInstance);

  /// Stops and deletes all coroutines on the specified instance.
  void StopAndDeleteAllCoroutines(nsScriptInstance* pInstance);

  /// Returns whether the coroutine has finished or been stopped.
  bool IsCoroutineFinished(nsScriptCoroutineHandle hCoroutine) const;

  ///@}

  /// Returns a shared expression VM for custom script implementations.
  ///
  /// The VM is NOT thread safe - only execute one expression at a time.
  nsExpressionVM& GetSharedExpressionVM() { return m_SharedExpressionVM; }

  /// Context information for scheduled script functions.
  struct FunctionContext
  {
    /// Flags controlling when the function should be executed.
    enum Flags : nsUInt8
    {
      None,              ///< Execute always
      OnlyWhenSimulating ///< Execute only during simulation
    };

    nsPointerWithFlags<const nsAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags == other.m_pFunctionAndFlags && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const nsWorldModule::UpdateContext& context);

  nsIntervalScheduler<FunctionContext> m_Scheduler;

  nsIdTable<nsScriptCoroutineId, nsUniquePtr<nsScriptCoroutine>> m_RunningScriptCoroutines;
  nsHashTable<nsScriptInstance*, nsSmallArray<nsScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  nsDynamicArray<nsUniquePtr<nsScriptCoroutine>> m_DeadScriptCoroutines;

  nsExpressionVM m_SharedExpressionVM;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct nsHashHelper<nsScriptWorldModule::FunctionContext>
{
  NS_ALWAYS_INLINE static nsUInt32 Hash(const nsScriptWorldModule::FunctionContext& value)
  {
    nsUInt32 hash = nsHashHelper<const void*>::Hash(value.m_pFunctionAndFlags);
    hash = nsHashingUtils::CombineHashValues32(hash, nsHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  NS_ALWAYS_INLINE static bool Equal(const nsScriptWorldModule::FunctionContext& a, const nsScriptWorldModule::FunctionContext& b) { return a == b; }
};
