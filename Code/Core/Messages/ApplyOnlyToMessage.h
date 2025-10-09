#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message used to restrict an operation to apply only to a specific game object.
///
/// This message carries a game object handle to specify which object should be affected
/// by a particular operation, allowing selective application of effects or behaviors.
struct NS_CORE_DLL nsMsgOnlyApplyToObject : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgOnlyApplyToObject, nsMessage);

  nsGameObjectHandle m_hObject;
};
