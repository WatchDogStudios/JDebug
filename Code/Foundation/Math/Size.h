#pragma once

#include <Foundation/Basics.h>

/// \brief Generic two-dimensional size representation with width and height components
///
/// Provides a simple container for representing rectangular dimensions in 2D space.
/// The template parameter allows using different numeric types (integers, floats) depending
/// on precision requirements. Common typedefs include nsSizeU32, nsSizeFloat, and nsSizeDouble.
/// Primarily used for representing viewport dimensions, texture sizes, and UI element bounds.
template <typename Type>
class nsSizeTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  NS_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type width;
  Type height;

  // *** Constructors ***
public:
  /// \brief Default constructor does not initialize the data.
  nsSizeTemplate();

  /// \brief Constructor to set all values.
  nsSizeTemplate(Type width, Type height);

  // *** Common Functions ***
public:
  /// \brief Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;
};

template <typename Type>
bool operator==(const nsSizeTemplate<Type>& v1, const nsSizeTemplate<Type>& v2);

template <typename Type>
bool operator!=(const nsSizeTemplate<Type>& v1, const nsSizeTemplate<Type>& v2);

#include <Foundation/Math/Implementation/Size_inl.h>

using nsSizeU32 = nsSizeTemplate<nsUInt32>;
using nsSizeFloat = nsSizeTemplate<float>;
using nsSizeDouble = nsSizeTemplate<double>;

NS_FOUNDATION_DLL nsStringView BuildString(char* szTmp, nsUInt32 uiLength, const nsSizeU32& arg);
