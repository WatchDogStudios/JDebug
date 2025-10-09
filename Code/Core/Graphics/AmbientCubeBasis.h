#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

/// \brief Defines the basis directions for ambient cube sampling.
///
/// Provides the six cardinal directions (positive/negative X, Y, Z) used for
/// ambient lighting calculations and directional sampling.
struct NS_CORE_DLL nsAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static nsVec3 s_Dirs[NumDirs];
};

/// \brief Template class for storing ambient lighting data in a cube format.
///
/// Stores lighting values for six directions (the cardinal axes) to approximate
/// ambient lighting. Values can be added via directional samples and evaluated
/// for any normal direction using trilinear interpolation.
template <typename T>
struct nsAmbientCube
{
  NS_DECLARE_POD_TYPE();

  nsAmbientCube();

  template <typename U>
  nsAmbientCube(const nsAmbientCube<U>& other);

  template <typename U>
  void operator=(const nsAmbientCube<U>& other);

  bool operator==(const nsAmbientCube& other) const;
  bool operator!=(const nsAmbientCube& other) const;

  void AddSample(const nsVec3& vDir, const T& value);

  T Evaluate(const nsVec3& vNormal) const;

  nsResult Serialize(nsStreamWriter& inout_stream) const;
  nsResult Deserialize(nsStreamReader& inout_stream);

  T m_Values[nsAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
