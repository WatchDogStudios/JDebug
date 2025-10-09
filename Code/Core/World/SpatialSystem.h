#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/Types/TagSet.h>

/// \brief Abstract base class for spatial systems that organize objects for efficient spatial queries.
///
/// Spatial systems manage spatial data for objects in a world, enabling efficient queries like
/// finding objects in a sphere or box, frustum culling, and visibility testing. Concrete
/// implementations use different spatial data structures (octrees, grids, etc.) for optimization.
class NS_CORE_DLL nsSpatialSystem : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(nsSpatialSystem, nsReflectedClass);

public:
  nsSpatialSystem();
  ~nsSpatialSystem();

  virtual void StartNewFrame();

  /// \name Spatial Data Functions
  ///@{

  virtual nsSpatialDataHandle CreateSpatialData(const nsSimdBBoxSphere& bounds, nsGameObject* pObject, nsUInt32 uiCategoryBitmask, const nsTagSet& tags) = 0;
  virtual nsSpatialDataHandle CreateSpatialDataAlwaysVisible(nsGameObject* pObject, nsUInt32 uiCategoryBitmask, const nsTagSet& tags) = 0;

  virtual void DeleteSpatialData(const nsSpatialDataHandle& hData) = 0;

  virtual void UpdateSpatialDataBounds(const nsSpatialDataHandle& hData, const nsSimdBBoxSphere& bounds) = 0;
  virtual void UpdateSpatialDataObject(const nsSpatialDataHandle& hData, nsGameObject* pObject) = 0;

  ///@}
  /// \name Simple Queries
  ///@{

  using QueryCallback = nsDelegate<nsVisitorExecution::Enum(nsGameObject*)>;

#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    nsUInt32 m_uiTotalNumObjects = 0;  ///< The total number of spatial objects in this system.
    nsUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    nsUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    nsTime m_TimeTaken;                ///< Time taken to execute the query
  };
#endif

  /// \brief Parameters for spatial queries to filter and track results.
  struct QueryParams
  {
    nsUInt32 m_uiCategoryBitmask = 0;         ///< Bitmask of spatial data categories to include in the query
    const nsTagSet* m_pIncludeTags = nullptr; ///< Only include objects that have all of these tags
    const nsTagSet* m_pExcludeTags = nullptr; ///< Exclude objects that have any of these tags
#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;           ///< Optional stats tracking for development builds
#endif
  };

  virtual void FindObjectsInSphere(const nsBoundingSphere& sphere, const QueryParams& queryParams, nsDynamicArray<nsGameObject*>& out_objects) const;
  virtual void FindObjectsInSphere(const nsBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const nsBoundingBox& box, const QueryParams& queryParams, nsDynamicArray<nsGameObject*>& out_objects) const;
  virtual void FindObjectsInBox(const nsBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = nsDelegate<bool(const nsSimdBBox&)>;

  virtual void FindVisibleObjects(const nsFrustum& frustum, const QueryParams& queryParams, nsDynamicArray<const nsGameObject*>& out_objects, IsOccludedFunc isOccluded, nsVisibilityState::Enum visType) const = 0;

  /// \brief Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  virtual nsVisibilityState::Enum GetVisibilityState(const nsSpatialDataHandle& hData, nsUInt32 uiNumFramesBeforeInvisible) const = 0;

  ///@}

#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(nsStringBuilder& ref_sSb) const;
#endif

protected:
  nsProxyAllocator m_Allocator;

  nsUInt64 m_uiFrameCounter = 0;
};

/// \brief Script extension class providing spatial query functions for scripting languages.
class NS_CORE_DLL nsScriptExtensionClass_Spatial
{
public:
  /// \brief Finds the closest object in a sphere within the given category.
  static nsGameObject* FindClosestObjectInSphere(nsWorld* pWorld, nsStringView sCategory, const nsVec3& vCenter, float fRadius);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_Spatial);
