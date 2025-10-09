#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message sent when a game object's global transform changes.
///
/// Contains both the old and new global transforms, allowing components to respond
/// to position, rotation, or scale changes and calculate movement deltas if needed.
struct NS_CORE_DLL nsMsgTransformChanged : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgTransformChanged, nsMessage);

  nsTransform m_OldGlobalTransform;
  nsTransform m_NewGlobalTransform;
};
