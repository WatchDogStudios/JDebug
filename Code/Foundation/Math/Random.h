#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief A random number generator. Currently uses the WELL512 algorithm.
class NS_FOUNDATION_DLL nsRandom
{
public:
  nsRandom();
  ~nsRandom();

  /// \brief Initializes the RNG with the given seed value. The value should not be zero.
  void Initialize(nsUInt64 uiSeed); // [tested]

  /// \brief Initializes the RNG using current time stamp.
  /// Not very sophisticated, but good enough for things that do not need to be secure.
  void InitializeFromCurrentTime();

  /// \brief Serializes the current state
  void Save(nsStreamWriter& inout_stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(nsStreamReader& inout_stream); // [tested]

  /// \brief Returns a uint32 value, ie. ranging from 0 to (2 ^ 32) - 1
  nsUInt32 UInt(); // [tested]

  /// \brief Returns a uint32 value in range [0 ; uiRange - 1]
  ///
  /// \note A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
  /// So always use a range of at least 1.
  nsUInt32 UIntInRange(nsUInt32 uiRange); // [tested]

  /// \brief Returns a random uint32 in range [0 ; uiArraySize - 1].
  ///
  /// If uiArraySize is 0, uiFallbackValue is returned instead.
  /// This is meant for generating a random index into an array.
  /// If the array is empty, no valid index can be generated, however, instead of asserting,
  /// a dedicated invalid index is returned.
  ///
  /// This function is nearly the same as UIntInRange(), except that that function asserts that uiRange is larger than 0.
  nsUInt32 UInt32Index(nsUInt32 uiArraySize, nsUInt32 uiFallbackValue = nsInvalidIndex);

  /// \brief Returns a random uint16 in range [0 ; uiArraySize - 1].
  ///
  /// Same as UInt32Index() just for 16 bit indices.
  nsUInt16 UInt16Index(nsUInt16 uiArraySize, nsUInt16 uiFallbackValue = 0xFFFF);

  /// \brief Returns an int32 value in range [iMinValue ; iMaxValue]
  nsInt32 IntMinMax(nsInt32 iMinValue, nsInt32 iMaxValue); // [tested]

  /// \brief Returns a boolean either being true or false
  NS_ALWAYS_INLINE bool Bool() { return static_cast<bool>(UInt() & 1); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0), ie. including zero, but excluding one
  NS_ALWAYS_INLINE double DoubleZeroToOneExclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0], ie. including zero and one
  NS_ALWAYS_INLINE double DoubleZeroToOneInclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL + 1.0); } // [tested]

  /// \brief Returns a double value in range [fMinValue ; fMaxValue]
  double DoubleMinMax(double fMinValue, double fMaxValue); // [tested]

  /// \brief Returns a double value around fValue with a given variance (0 - 1 range)
  double DoubleVariance(double fValue, double fVariance);

  /// \brief Returns a double value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
  double DoubleVarianceAroundZero(double fAbsMaxValue);

  /// \brief Returns a value in range [0.0 ; 1.0), ie. including zero, but excluding one
  NS_ALWAYS_INLINE float FloatZeroToOneExclusive() { return static_cast<float>(DoubleZeroToOneExclusive()); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0], ie. including zero and one
  NS_ALWAYS_INLINE float FloatZeroToOneInclusive() { return static_cast<float>(DoubleZeroToOneInclusive()); } // [tested]

  /// \brief Returns a float value in range [fMinValue ; fMaxValue]
  NS_ALWAYS_INLINE float FloatMinMax(float fMinValue, float fMaxValue) { return static_cast<float>(DoubleMinMax(fMinValue, fMaxValue)); } // [tested]

  /// \brief Returns a float value around fValue with a given variance (0 - 1 range)
  NS_ALWAYS_INLINE float FloatVariance(float fValue, float fVariance) { return static_cast<float>(DoubleVariance(fValue, fVariance)); }

  /// \brief Returns a float value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
  NS_ALWAYS_INLINE float FloatVarianceAroundZero(float fAbsMaxValue) { return static_cast<float>(DoubleVarianceAroundZero(fAbsMaxValue)); }

private:
  nsUInt32 m_uiIndex = 0;
  nsUInt32 m_uiState[16];
};

NS_DECLARE_REFLECTABLE_TYPE(NS_FOUNDATION_DLL, nsRandom);


/// \brief A random number generator that produces values with a normal / Gaussian distribution
class NS_FOUNDATION_DLL nsRandomGauss
{
public:
  /// \brief Initializes the RNG and sets the maximum value that the functions UnsignedValue() and SignedValue() may return
  ///
  /// The Variance configures the distribution of the samples. 1.0 gives a standard bell-curve. Values below 1 lead to a distribution
  /// with more emphasis around zero, whereas values above 1 result in a flatter curve with more equally distributed results.
  ///
  /// For more details, look here: https://en.wikipedia.org/wiki/Normal_distribution
  void Initialize(nsUInt64 uiRandomSeed, nsUInt32 uiMaxValue, float fVariance = 1.0f); // [tested]

  /// \brief Returns a value in range [0; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than uiMaxValue.
  nsUInt32 UnsignedValue(); // [tested]

  /// \brief Returns a value in range [-uiMaxValue + 1; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than +/-uiMaxValue.
  nsInt32 SignedValue(); // [tested]

  /// \brief Serializes the current state
  void Save(nsStreamWriter& inout_stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(nsStreamReader& inout_stream); // [tested]

private:
  void SetupTable(nsUInt32 uiMaxValue, float fSigma);

  float m_fSigma;
  double m_fAreaSum;
  nsDynamicArray<float> m_GaussAreaSum;
  nsRandom m_Generator;
};

#include <Foundation/Math/Implementation/AllClassesRandom_inl.h>
