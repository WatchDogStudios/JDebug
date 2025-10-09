#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
NS_BEGIN_COMPONENT_TYPE(nsForwardEventsToGameStateComponent, 1 /* version */, nsComponentMode::Static)
{
  NS_BEGIN_ATTRIBUTES
  {
    new nsCategoryAttribute("Logic"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_COMPONENT_TYPE
// clang-format on

nsForwardEventsToGameStateComponent::nsForwardEventsToGameStateComponent() = default;
nsForwardEventsToGameStateComponent::~nsForwardEventsToGameStateComponent() = default;

bool nsForwardEventsToGameStateComponent::HandlesMessage(const nsMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (nsGameStateBase* pGameState = nsGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool nsForwardEventsToGameStateComponent::OnUnhandledMessage(nsMessage& msg, bool bWasPostedMsg)
{
  NS_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (nsGameStateBase* pGameState = nsGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool nsForwardEventsToGameStateComponent::OnUnhandledMessage(nsMessage& msg, bool bWasPostedMsg) const
{
  NS_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (const nsGameStateBase* pGameState = nsGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void nsForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}


NS_STATICLINK_FILE(Core, Core_GameState_Implementation_ForwardEventsToGameStateComponent);
