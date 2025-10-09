#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using nsScriptComponentManager = nsComponentManager<class nsScriptComponent, nsBlockStorageType::FreeList>;

/// \brief Component that hosts and executes a script class instance on a game object.
///
/// Manages script execution lifecycle, variable access, parameter exposure, and event handling.
/// Supports configurable update intervals and simulation-only updates. Provides integration
/// between game objects and scripting systems through the nsScriptClassResource.
class NS_CORE_DLL nsScriptComponent : public nsEventMessageHandlerComponent
{
  NS_DECLARE_COMPONENT_TYPE(nsScriptComponent, nsEventMessageHandlerComponent, nsScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // nsComponent

protected:
  virtual void SerializeComponent(nsWorldWriter& stream) const override;
  virtual void DeserializeComponent(nsWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // nsScriptComponent
public:
  nsScriptComponent();
  ~nsScriptComponent();

  void SetScriptVariable(const nsHashedString& sName, const nsVariant& value);         // [ scriptable ]
  nsVariant GetScriptVariable(const nsHashedString& sName) const;                      // [ scriptable ]

  void SetScriptClass(const nsScriptClassResourceHandle& hScript);                     // [ property ]
  const nsScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; } // [ property ]

  void SetUpdateInterval(nsTime interval);                                             // [ property ]
  nsTime GetUpdateInterval() const { return m_UpdateInterval; }                        // [ property ]

  void SetUpdateOnlyWhenSimulating(bool bUpdate);                                      // [ property ]
  bool GetUpdateOnlyWhenSimulating() const { return m_bUpdateOnlyWhenSimulating; }     // [ property ]

  void BroadcastEventMsg(nsEventMessage& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const nsRangeView<const char*, nsUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const nsVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, nsVariant& out_value) const;

  NS_ALWAYS_INLINE nsScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void AddUpdateFunctionToSchedule();
  void RemoveUpdateFunctionToSchedule();

  const nsAbstractFunctionProperty* GetScriptFunction(nsUInt32 uiFunctionIndex);
  void CallScriptFunction(nsUInt32 uiFunctionIndex);

  void ReloadScript();

  nsArrayMap<nsHashedString, nsVariant> m_Parameters;

  nsScriptClassResourceHandle m_hScriptClass;
  nsTime m_UpdateInterval = nsTime::MakeZero();
  bool m_bUpdateOnlyWhenSimulating = true;

  nsSharedPtr<nsScriptRTTI> m_pScriptType;
  nsUniquePtr<nsScriptInstance> m_pInstance;

private:
  struct EventSender
  {
    const nsRTTI* m_pMsgType = nullptr;
    nsEventMessageSender<nsEventMessage> m_Sender;
  };

  nsSmallArray<EventSender, 1> m_EventSenders;
};
