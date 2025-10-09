#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message sent when a game object's parent relationship changes.
///
/// Notifies components when their object is linked to or unlinked from a parent object.
struct NS_CORE_DLL nsMsgParentChanged : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgParentChanged, nsMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
    Invalid
  };

  Type m_Type = Type::Invalid;
  nsGameObjectHandle m_hParent; // previous or new parent, depending on m_Type
};

/// \brief Message sent when a game object's children change.
///
/// Notifies parent objects when child objects are added or removed from their hierarchy.
struct NS_CORE_DLL nsMsgChildrenChanged : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgChildrenChanged, nsMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type m_Type;
  nsGameObjectHandle m_hParent;
  nsGameObjectHandle m_hChild;
};

/// \brief Message sent when components are added to or removed from a game object.
///
/// Notifies interested parties when the component composition of an object changes.
struct NS_CORE_DLL nsMsgComponentsChanged : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgComponentsChanged, nsMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved,
    Invalid
  };

  Type m_Type = Type::Invalid;
  nsGameObjectHandle m_hOwner;
  nsComponentHandle m_hComponent;
};
