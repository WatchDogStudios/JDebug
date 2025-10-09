#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class nsWorld;

/// \brief Defines the different phases during world updates for module execution ordering.
struct nsWorldUpdatePhase
{
  using StorageType = nsUInt8;

  enum Enum
  {
    PreAsync,      ///< Synchronous phase before parallel processing
    Async,         ///< Parallel processing phase (thread-safe operations only)
    PostAsync,     ///< Synchronous phase after parallel processing
    PostTransform, ///< Synchronous phase after transform updates
    COUNT,

    Default = PreAsync
  };
};

/// \brief Base class for world modules that extend world functionality.
///
/// World modules provide additional functionality to worlds such as component management,
/// physics simulation, or rendering. They can register update functions that are called
/// during different phases of the world update cycle and manage resources and state.
class NS_CORE_DLL nsWorldModule : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsWorldModule, nsReflectedClass);

protected:
  nsWorldModule(nsWorld* pWorld);
  virtual ~nsWorldModule();

public:
  /// \brief Returns the corresponding world to this module.
  nsWorld* GetWorld();

  /// \brief Returns the corresponding world to this module.
  const nsWorld* GetWorld() const;

  /// \brief Same as GetWorld()->GetIndex(). Needed to break circular include dependencies.
  nsUInt32 GetWorldIndex() const;

protected:
  friend class nsWorld;
  friend class nsInternal::WorldData;
  friend class nsMemoryUtils;

  /// \brief Context passed to update functions containing information about component range to process.
  struct UpdateContext
  {
    nsUInt32 m_uiFirstComponentIndex = 0; ///< Index of the first component to process in this batch
    nsUInt32 m_uiComponentCount = 0;      ///< Number of components to process in this batch
  };

  /// \brief Update function delegate.
  using UpdateFunction = nsDelegate<void(const UpdateContext&)>;

  /// \brief Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    UpdateFunctionDesc(const UpdateFunction& function, nsStringView sFunctionName)
      : m_Function(function)
    {
      m_sFunctionName.Assign(sFunctionName);
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    nsHashedString m_sFunctionName;               ///< Name of the function. Use the NS_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                                  ///< with the correct name.
    nsHybridArray<nsHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    nsEnum<nsWorldUpdatePhase> m_Phase;           ///< The update phase in which this update function should be called. See nsWorld for a description on the different phases.
    bool m_bOnlyUpdateWhenSimulating = false;     ///< The update function is only called when the world simulation is enabled.
    nsUInt16 m_uiAsyncPhaseBatchSize = 0;         ///< 0 means m_Function is called once per frame, to update all components, but still in parallel with other world modules.
                                                  ///< >0 means m_Function is called multiple times (in parallel) with batches of roughly this size.
    float m_fPriority = 0.0f;                     ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// \brief Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Returns the allocator used by the world.
  nsAllocator* GetAllocator();

  /// \brief Returns the block allocator used by the world.
  nsInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
  /// \brief This method is called after the constructor. A derived type can override this method to do initialization work. Typically this
  /// is the method where updates function are registered.
  virtual void Initialize() {}

  /// \brief This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual void Deinitialize() {}

  /// \brief This method is called at the start of the next world update when the world is simulated. This method will be called after the
  /// initialization method.
  virtual void OnSimulationStarted() {}

  /// \brief Called by nsWorld::Clear(). Can be used to clear cached data when a world is completely cleared of objects (but not deleted).
  virtual void WorldClear() {}

  nsWorld* m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to get component type ids and create new instances of world modules from rtti.
class NS_CORE_DLL nsWorldModuleFactory
{
public:
  static nsWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  nsWorldModuleTypeId RegisterWorldModule();

  /// \brief Returns the module type id to the given rtti module/component type.
  nsWorldModuleTypeId GetTypeId(const nsRTTI* pRtti);

  /// \brief Creates a new instance of the world module with the given type id and world.
  nsWorldModule* CreateWorldModule(nsUInt16 uiTypeId, nsWorld* pWorld);

  /// \brief Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(nsStringView sInterfaceName, nsStringView sImplementationName);

private:
  NS_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  using CreatorFunc = nsWorldModule* (*)(nsAllocator*, nsWorld*);

  nsWorldModuleFactory();
  nsWorldModuleTypeId RegisterWorldModule(const nsRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const nsPluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();
  void AdjustBaseTypeId(const nsRTTI* pParentRtti, const nsRTTI* pRtti, nsUInt16 uiParentTypeId);

  nsHashTable<const nsRTTI*, nsWorldModuleTypeId> m_TypeToId;

  struct CreatorFuncContext
  {
    NS_DECLARE_POD_TYPE();

    CreatorFunc m_Func;
    const nsRTTI* m_pRtti;
  };

  nsDynamicArray<CreatorFuncContext> m_CreatorFuncs;

  nsHashTable<nsString, nsString> m_InterfaceImplementations;
};

/// \brief Add this macro to the declaration of your module type.
#define NS_DECLARE_WORLD_MODULE()                      \
public:                                                \
  static NS_ALWAYS_INLINE nsWorldModuleTypeId TypeId() \
  {                                                    \
    return s_TypeId;                                   \
  }                                                    \
                                                       \
private:                                               \
  static nsWorldModuleTypeId s_TypeId;

/// \brief Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define NS_IMPLEMENT_WORLD_MODULE(moduleType) \
  nsWorldModuleTypeId moduleType::s_TypeId = nsWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// \brief Helper macro to create an update function description with proper name
#define NS_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) nsWorldModule::UpdateFunctionDesc(nsWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
