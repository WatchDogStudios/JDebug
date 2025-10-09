#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/World/WorldModule.h>

/// A triangle used for navmesh generation.
struct nsNavmeshTriangle
{
  nsVec3 m_Vertices[3];
  const nsSurfaceResource* m_pSurface = nullptr;
};

/// \brief A world module that retrieves triangle data that should be used for building navmeshes at runtime.
///
/// If a physics engine is active, it usually automatically provides such a world module to retrieve the triangle data
/// through physics queries.
///
/// In other types of games, a custom world module can be implemented, to generate this data in a different way.
/// If a physics engine is active, but a custom method should be used, you can write a custom world module
/// and then use nsWorldModuleFactory::RegisterInterfaceImplementation() to specify which module to use.
/// Also see nsWorldModuleConfig.
class NS_CORE_DLL nsNavmeshGeoWorldModuleInterface : public nsWorldModule
{
  NS_ADD_DYNAMIC_REFLECTION(nsNavmeshGeoWorldModuleInterface, nsWorldModule);

protected:
  nsNavmeshGeoWorldModuleInterface(nsWorld* pWorld)
    : nsWorldModule(pWorld)
  {
  }

public:
  /// Retrieves triangles within a specified area for navmesh generation.
  ///
  /// \param uiCollisionLayer The collision layer to query for geometry
  /// \param box The bounding box defining the area to retrieve geometry from
  /// \param out_triangles Array to fill with triangles found in the specified area
  virtual void RetrieveGeometryInArea(nsUInt32 uiCollisionLayer, const nsBoundingBox& box, nsDynamicArray<nsNavmeshTriangle>& out_triangles) const = 0;
};
