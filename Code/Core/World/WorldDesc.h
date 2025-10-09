#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

class nsTimeStepSmoothing;

/// \brief Describes the initial state of a world.
struct nsWorldDesc
{
  NS_DECLARE_POD_TYPE();

  nsWorldDesc(nsStringView sWorldName) { m_sName.Assign(sWorldName); }

  nsHashedString m_sName;                                                         ///< Name of the world for identification
  nsUInt64 m_uiRandomNumberGeneratorSeed = 0;                                     ///< Seed for the world's random number generator (0 = use current time)

  nsUniquePtr<nsSpatialSystem> m_pSpatialSystem;                                  ///< Custom spatial system to use for this world
  bool m_bAutoCreateSpatialSystem = true;                                         ///< Automatically create a default spatial system if none is set

  nsSharedPtr<nsCoordinateSystemProvider> m_pCoordinateSystemProvider;            ///< Optional provider for position-dependent coordinate systems
  nsUniquePtr<nsTimeStepSmoothing> m_pTimeStepSmoothing;                          ///< Custom time step smoothing (if nullptr, nsDefaultTimeStepSmoothing will be used)

  bool m_bReportErrorWhenStaticObjectMoves = true;                                ///< Whether to log errors when objects marked as static change position

  nsTime m_MaxComponentInitializationTimePerFrame = nsTime::MakeFromHours(10000); ///< Maximum time to spend on component initialization per frame
};
