#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

/// Message sent to components to gather their local bounds for spatial data systems.
///
/// Components receive this message to contribute their local bounding volumes
/// to the game object's overall bounds. Multiple components can add their bounds
/// which are accumulated into a single result used for culling and spatial queries.
struct NS_CORE_DLL nsMsgUpdateLocalBounds : public nsMessage
{
  NS_DECLARE_MESSAGE_TYPE(nsMsgUpdateLocalBounds, nsMessage);

  /// Adds bounding volume to the accumulated local bounds.
  ///
  /// \param bounds The local bounding volume to include
  /// \param category The spatial data category this bounds belongs to
  NS_ALWAYS_INLINE void AddBounds(const nsBoundingBoxSphere& bounds, nsSpatialData::Category category)
  {
    m_ResultingLocalBounds.ExpandToInclude(bounds);
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

  /// Marks the object as always visible, bypassing culling systems.
  ///
  /// Once set, this flag cannot be unset during the same message handling,
  /// as the message accumulates data from multiple components.
  /// \param category The spatial data category for the always-visible flag
  NS_ALWAYS_INLINE void SetAlwaysVisible(nsSpatialData::Category category)
  {
    m_bAlwaysVisible = true;
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

private:
  friend class nsGameObject;

  nsBoundingBoxSphere m_ResultingLocalBounds;
  nsUInt32 m_uiSpatialDataCategoryBitmask = 0;
  bool m_bAlwaysVisible = false;
};
