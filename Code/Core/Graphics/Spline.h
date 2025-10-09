#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdTransform.h>

class nsStreamReader;
class nsStreamWriter;

/// \brief The different modes that tangents may use in a spline control point.
struct nsSplineTangentMode
{
  using StorageType = nsUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the control point is automatically computed to be smooth.
    Custom, ///< Custom tangents specified by the user.
    Linear, ///< There is no curvature through this control point/tangent. Creates sharp corners.

    Default = Auto
  };
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsSplineTangentMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Describes a spline consisting of cubic Bnsier curves segments. Each control point defines the position, rotation, and scale at that point.
/// The parameter fT to evaluate the spline is a combination of the control point index and the zero to one parameter in the fractional part to interpolate within that segment.
struct NS_CORE_DLL nsSpline
{
  struct ControlPoint
  {
    nsSimdVec4f m_vPos = nsSimdVec4f::MakeZero();
    nsSimdVec4f m_vPosTangentIn = nsSimdVec4f::MakeZero();  // Contains the tangent mode in w
    nsSimdVec4f m_vPosTangentOut = nsSimdVec4f::MakeZero(); // Contains the tangent mode in w

    nsSimdVec4f m_vUpDirAndRoll = nsSimdVec4f::MakeZero();  // Roll angle in w
    nsSimdVec4f m_vUpDirTangentIn = nsSimdVec4f::MakeZero();
    nsSimdVec4f m_vUpDirTangentOut = nsSimdVec4f::MakeZero();

    nsSimdVec4f m_vScale = nsSimdVec4f(1);
    nsSimdVec4f m_vScaleTangentIn = nsSimdVec4f::MakeZero();
    nsSimdVec4f m_vScaleTangentOut = nsSimdVec4f::MakeZero();

    nsResult Serialize(nsStreamWriter& ref_writer) const;
    nsResult Deserialize(nsStreamReader& ref_reader);

    void SetPosition(const nsSimdVec4f& vPos);

    nsSplineTangentMode::Enum GetTangentModeIn() const;
    void SetTangentModeIn(nsSplineTangentMode::Enum mode);
    void SetTangentIn(const nsSimdVec4f& vTangent, nsSplineTangentMode::Enum mode = nsSplineTangentMode::Custom);

    nsSplineTangentMode::Enum GetTangentModeOut() const;
    void SetTangentModeOut(nsSplineTangentMode::Enum mode);
    void SetTangentOut(const nsSimdVec4f& vTangent, nsSplineTangentMode::Enum mode = nsSplineTangentMode::Custom);

    nsAngle GetRoll() const;
    void SetRoll(nsAngle roll);

    void SetScale(const nsSimdVec4f& vScale);

    void SetAutoTangents(const nsSimdVec4f& vDirIn, const nsSimdVec4f& vDirOut);
  };

  nsDynamicArray<ControlPoint, nsAlignedAllocatorWrapper> m_ControlPoints;
  bool m_bClosed = false;
  nsUInt32 m_uiChangeCounter = 0; //< Not incremented automatically, but can be incremented by the user to signal that the spline has changed.

  nsResult Serialize(nsStreamWriter& ref_writer) const;
  nsResult Deserialize(nsStreamReader& ref_reader);

  /// \brief Calculates tangents for all control points with a tangent mode other than 'Custom'.
  void CalculateUpDirAndAutoTangents(const nsSimdVec4f& vGlobalUpDir = nsSimdVec4f(0, 0, 1), const nsSimdVec4f& vGlobalForwardDir = nsSimdVec4f(1, 0, 0));

  /// \brief Returns the position of the spline at the given parameter fT.
  nsSimdVec4f EvaluatePosition(float fT) const;
  nsSimdVec4f EvaluatePosition(nsUInt32 uiCp0, const nsSimdFloat& fT) const;

  /// \brief Returns the derivative, aka the tangent of the spline at the given parameter fT. This also equals to the unnormalized forward direction.
  nsSimdVec4f EvaluateDerivative(float fT) const;
  nsSimdVec4f EvaluateDerivative(nsUInt32 uiCp0, const nsSimdFloat& fT) const;

  /// \brief Returns the up direction of the spline at the given parameter fT.
  nsSimdVec4f EvaluateUpDirection(float fT) const;

  /// \brief Returns the scale of the spline at the given parameter fT.
  nsSimdVec4f EvaluateScale(float fT) const;

  /// \brief Returns the full transform (consisting of position, scale, and orientation) of the spline at the given parameter fT.
  nsSimdTransform EvaluateTransform(float fT) const;

private:
  float ClampAndSplitT(float fT, nsUInt32& out_uiIndex) const;
  nsUInt32 GetCp1Index(nsUInt32 uiCp0) const;

  nsSimdVec4f EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const;
  nsSimdVec4f EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const;

  void EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT, nsSimdVec4f& out_forwardDir, nsSimdVec4f& out_rightDir, nsSimdVec4f& out_upDir) const;

  nsSimdVec4f EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const;
};

#include <Core/Graphics/Implementation/Spline_inl.h>
