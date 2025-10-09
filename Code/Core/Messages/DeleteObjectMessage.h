#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

/// \brief Message to request deletion of a game object and optionally its empty parent objects.
///
/// When sent to a game object, this message will cause it to be deleted. Can also clean up
/// empty parent objects in the hierarchy and provides cancellation capability for components
/// that need to orchestrate the deletion timing.
struct NS_CORE_DLL nsMsgDeleteGameObject : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgDeleteGameObject, nsMessage);

  /// \brief If set to true, any parent/ancestor that has no other children or components will also be deleted.
  bool m_bDeleteEmptyParents = true;

  /// \brief This is used by nsOnComponentFinishedAction to orchestrate when an object shall really be deleted.
  bool m_bCancel = false;
};
