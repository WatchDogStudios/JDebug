#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

struct nsDynamicTree
{
  struct nsObjectData
  {
    nsInt32 m_iObjectType;
    nsInt32 m_iObjectInstance;
  };

  struct nsMultiMapKey
  {
    nsUInt32 m_uiKey;
    nsUInt32 m_uiCounter;

    nsMultiMapKey()
    {
      m_uiKey = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const nsMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const nsMultiMapKey& rhs) const { return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey); }
  };
};

using nsDynamicTreeObject = nsMap<nsDynamicTree::nsMultiMapKey, nsDynamicTree::nsObjectData>::Iterator;
using nsDynamicTreeObjectConst = nsMap<nsDynamicTree::nsMultiMapKey, nsDynamicTree::nsObjectData>::ConstIterator;

/// \brief Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
using NS_VISIBLE_OBJ_CALLBACK = bool (*)(void*, nsDynamicTreeObjectConst);

class nsDynamicOctree;
class nsDynamicQuadtree;
