#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Spline.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/SimdMath/SimdConversion.h>

// clang-format off
NS_BEGIN_STATIC_REFLECTED_ENUM(nsSplineTangentMode, 1)
  NS_ENUM_CONSTANTS(nsSplineTangentMode::Auto, nsSplineTangentMode::Custom, nsSplineTangentMode::Linear)
NS_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

nsResult nsSpline::ControlPoint::Serialize(nsStreamWriter& s) const
{
  s << nsSimdConversion::ToVec3(m_vPos);
  s << nsSimdConversion::ToVec4(m_vPosTangentIn);  // Contains the tangent mode in w
  s << nsSimdConversion::ToVec4(m_vPosTangentOut); // Contains the tangent mode in w

  s << nsSimdConversion::ToVec4(m_vUpDirAndRoll);  // Roll in w
  s << nsSimdConversion::ToVec3(m_vUpDirTangentIn);
  s << nsSimdConversion::ToVec3(m_vUpDirTangentOut);

  s << nsSimdConversion::ToVec3(m_vScale);
  s << nsSimdConversion::ToVec3(m_vScaleTangentIn);
  s << nsSimdConversion::ToVec3(m_vScaleTangentOut);

  return NS_SUCCESS;
}

nsResult nsSpline::ControlPoint::Deserialize(nsStreamReader& s)
{
  {
    nsVec3 vPos;
    s >> vPos;

    nsVec4 vPosTangentIn, vPosTangentOut; // Contains the tangent mode in w
    s >> vPosTangentIn;
    s >> vPosTangentOut;

    m_vPos = nsSimdConversion::ToVec3(vPos);
    m_vPosTangentIn = nsSimdConversion::ToVec4(vPosTangentIn);
    m_vPosTangentOut = nsSimdConversion::ToVec4(vPosTangentOut);
  }

  {
    nsVec4 vUpDirAndRoll; // Roll in w
    s >> vUpDirAndRoll;

    nsVec3 vUpDirTangentIn, vUpDirTangentOut;
    s >> vUpDirTangentIn;
    s >> vUpDirTangentOut;

    m_vUpDirAndRoll = nsSimdConversion::ToVec4(vUpDirAndRoll);
    m_vUpDirTangentIn = nsSimdConversion::ToVec3(vUpDirTangentIn);
    m_vUpDirTangentOut = nsSimdConversion::ToVec3(vUpDirTangentOut);
  }

  {
    nsVec3 vScale, vScaleTangentIn, vScaleTangentOut;
    s >> vScale;
    s >> vScaleTangentIn;
    s >> vScaleTangentOut;

    m_vScale = nsSimdConversion::ToVec3(vScale);
    m_vScaleTangentIn = nsSimdConversion::ToVec3(vScaleTangentIn);
    m_vScaleTangentOut = nsSimdConversion::ToVec3(vScaleTangentOut);
  }

  return NS_SUCCESS;
}

void nsSpline::ControlPoint::SetAutoTangents(const nsSimdVec4f& vDirIn, const nsSimdVec4f& vDirOut)
{
  const nsSimdVec4f autoPosTangent = (vDirIn + vDirOut) * 0.5f;
  const nsSimdFloat eps = nsMath::LargeEpsilon<float>();

  {
    auto tangentModeIn = GetTangentModeIn();
    if (tangentModeIn == nsSplineTangentMode::Auto)
    {
      m_vPosTangentIn = -autoPosTangent;
    }
    else if (tangentModeIn == nsSplineTangentMode::Linear)
    {
      m_vPosTangentIn = -vDirIn;
    }
    else
    {
      NS_ASSERT_DEV(tangentModeIn == nsSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentIn.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentIn = vDirIn;
      m_vPosTangentIn.NormalizeIfNotZero<3>(nsSimdVec4f(-1, 0, 0));
      m_vPosTangentIn *= eps;
    }

    SetTangentModeIn(nsSplineTangentMode::Custom);
  }

  {
    auto tangentModeOut = GetTangentModeOut();
    if (tangentModeOut == nsSplineTangentMode::Auto)
    {
      m_vPosTangentOut = autoPosTangent;
    }
    else if (tangentModeOut == nsSplineTangentMode::Linear)
    {
      m_vPosTangentOut = vDirOut;
    }
    else
    {
      NS_ASSERT_DEV(tangentModeOut == nsSplineTangentMode::Custom, "Unknown spline tangent mode");
    }

    // Sanitize tangent
    if (m_vPosTangentOut.GetLengthSquared<3>() < eps)
    {
      m_vPosTangentOut = vDirOut;
      m_vPosTangentOut.NormalizeIfNotZero<3>(nsSimdVec4f(1, 0, 0));
      m_vPosTangentOut *= eps;
    }

    SetTangentModeOut(nsSplineTangentMode::Custom);
  }
}

//////////////////////////////////////////////////////////////////////////

constexpr nsTypeVersion s_SplineVersion = 1;

nsResult nsSpline::Serialize(nsStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_SplineVersion);

  NS_SUCCEED_OR_RETURN(ref_writer.WriteArray(m_ControlPoints));
  ref_writer << m_bClosed;

  return NS_SUCCESS;
}

nsResult nsSpline::Deserialize(nsStreamReader& ref_reader)
{
  /*const nsTypeVersion version =*/ref_reader.ReadVersion(s_SplineVersion);

  NS_SUCCEED_OR_RETURN(ref_reader.ReadArray(m_ControlPoints));
  ref_reader >> m_bClosed;

  return NS_SUCCESS;
}

void nsSpline::CalculateUpDirAndAutoTangents(const nsSimdVec4f& vGlobalUpDir, const nsSimdVec4f& vGlobalForwardDir)
{
  const nsUInt32 uiNumPoints = m_ControlPoints.GetCount();
  if (uiNumPoints < 2)
    return;

  const nsUInt32 uiLastIdx = uiNumPoints - 1;
  const nsSimdFloat oneThird(1.0f / 3.0f);

  // Position tangents
  {
    nsUInt32 uiNumTangentsToUpdate = uiNumPoints;
    nsUInt32 uiPrevIdx = uiLastIdx - 1;
    nsUInt32 uiCurIdx = uiLastIdx;
    nsUInt32 uiNextIdx = 0;

    if (!m_bClosed)
    {
      const nsSimdVec4f vStartTangent = (m_ControlPoints[1].m_vPos - m_ControlPoints[0].m_vPos) * oneThird;
      const nsSimdVec4f vEndTangent = (m_ControlPoints[uiLastIdx].m_vPos - m_ControlPoints[uiLastIdx - 1].m_vPos) * oneThird;

      m_ControlPoints[0].SetAutoTangents(vStartTangent, vStartTangent);
      m_ControlPoints[uiLastIdx].SetAutoTangents(vEndTangent, vEndTangent);

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (nsUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      const nsSimdVec4f dirIn = (cCp.m_vPos - pCP.m_vPos) * oneThird;
      const nsSimdVec4f dirOut = (nCP.m_vPos - cCp.m_vPos) * oneThird;

      cCp.SetAutoTangents(dirIn, dirOut);

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }

  // Up dir
  {
    for (nsUInt32 i = 0; i < uiNumPoints; ++i)
    {
      auto& cp = m_ControlPoints[i];

      nsSimdVec4f forwardDir = EvaluateDerivative(i, 0.0f);
      forwardDir.NormalizeIfNotZero<3>(vGlobalForwardDir);

      const nsSimdVec4f upDir = [&]()
      {
        if (!forwardDir.IsEqual(vGlobalUpDir, nsMath::HugeEpsilon<float>()).AllSet<3>())
          return vGlobalUpDir;

        if (i > 0)
        {
          auto& prevCp = m_ControlPoints[i - 1];
          if (!forwardDir.IsEqual(prevCp.m_vUpDirAndRoll, nsMath::HugeEpsilon<float>()).AllSet<3>())
          {
            return prevCp.m_vUpDirAndRoll;
          }
        }

        return vGlobalForwardDir;
      }();

      const nsSimdVec4f rightDir = upDir.CrossRH(forwardDir).GetNormalized<3>();
      const nsSimdVec4f upDir2 = forwardDir.CrossRH(rightDir).GetNormalized<3>();
      const nsSimdFloat roll = cp.GetRoll();
      const nsSimdQuat rotation = nsSimdQuat::MakeFromAxisAndAngle(forwardDir, roll);

      cp.m_vUpDirAndRoll = rotation * upDir2;
      cp.m_vUpDirAndRoll.SetW(roll);
      cp.m_vUpDirTangentIn.SetZero();
      cp.m_vUpDirTangentOut.SetZero();
    }
  }

  // up dir and scale tangents
  {
    nsUInt32 uiNumTangentsToUpdate = uiNumPoints;
    nsUInt32 uiPrevIdx = uiLastIdx - 1;
    nsUInt32 uiCurIdx = uiLastIdx;
    nsUInt32 uiNextIdx = 0;


    if (!m_bClosed)
    {
      {
        auto& cp0 = m_ControlPoints[0];
        auto& cp1 = m_ControlPoints[1];

        const nsSimdVec4f vUpDirTangent = (cp1.m_vUpDirAndRoll - cp0.m_vUpDirAndRoll) * oneThird;
        const nsSimdVec4f vScaleTangent = (cp1.m_vScale - cp0.m_vScale) * oneThird;

        cp0.m_vUpDirTangentIn = -vUpDirTangent;
        cp0.m_vUpDirTangentOut = vUpDirTangent;
        cp0.m_vScaleTangentIn = -vScaleTangent;
        cp0.m_vScaleTangentOut = vScaleTangent;
      }

      {
        auto& cpLast = m_ControlPoints[uiLastIdx];
        auto& cpPrev = m_ControlPoints[uiLastIdx - 1];

        const nsSimdVec4f vUpDirTangent = (cpLast.m_vUpDirAndRoll - cpPrev.m_vUpDirAndRoll) * oneThird;
        const nsSimdVec4f vScaleTangent = (cpLast.m_vScale - cpPrev.m_vScale) * oneThird;

        cpLast.m_vUpDirTangentIn = -vUpDirTangent;
        cpLast.m_vUpDirTangentOut = vUpDirTangent;
        cpLast.m_vScaleTangentIn = -vScaleTangent;
        cpLast.m_vScaleTangentOut = vScaleTangent;
      }

      uiNumTangentsToUpdate = uiNumPoints - 2;
      uiPrevIdx = 0;
      uiCurIdx = 1;
      uiNextIdx = 2;
    }

    for (nsUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
    {
      auto& cCp = m_ControlPoints[uiCurIdx];
      const auto& pCP = m_ControlPoints[uiPrevIdx];
      const auto& nCP = m_ControlPoints[uiNextIdx];

      // Do not use classic auto tangents here, since we don't want overshooting for the up direction and scale.
      const nsSimdVec4f vUpDirTangent = (cCp.m_vUpDirAndRoll - pCP.m_vUpDirAndRoll).CompMin(nCP.m_vUpDirAndRoll - cCp.m_vUpDirAndRoll) * oneThird;
      const nsSimdVec4f vScaleTangent = (cCp.m_vScale - pCP.m_vScale).CompMin(nCP.m_vScale - cCp.m_vScale) * oneThird;

      cCp.m_vUpDirTangentIn = -vUpDirTangent;
      cCp.m_vUpDirTangentOut = vUpDirTangent;
      cCp.m_vScaleTangentIn = -vScaleTangent;
      cCp.m_vScaleTangentOut = vScaleTangent;

      uiPrevIdx = uiCurIdx;
      uiCurIdx = uiNextIdx;
      ++uiNextIdx;
    }
  }
}

nsSimdTransform nsSpline::EvaluateTransform(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return nsSimdTransform::MakeIdentity();

  nsUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const nsUInt32 uiCp1 = GetCp1Index(uiCp0);
  const auto& cp0 = m_ControlPoints[uiCp0];
  const auto& cp1 = m_ControlPoints[uiCp1];

  nsSimdTransform transform;
  transform.m_Position = EvaluatePosition(cp0, cp1, fT);

  nsSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(cp0, cp1, fT, forwardDir, rightDir, upDir);

  nsMat3 mRot;
  mRot.SetColumn(0, nsSimdConversion::ToVec3(forwardDir));
  mRot.SetColumn(1, nsSimdConversion::ToVec3(rightDir));
  mRot.SetColumn(2, nsSimdConversion::ToVec3(upDir));
  transform.m_Rotation = nsSimdConversion::ToQuat(nsQuat::MakeFromMat3(mRot));

  transform.m_Scale = EvaluateScale(cp0, cp1, fT);

  return transform;
}


NS_STATICLINK_FILE(Core, Core_Graphics_Implementation_Spline);
