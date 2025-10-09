#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const nsCoordinateSystemConversion& atoB, const nsCoordinateSystemConversion& btoA, float fSourceLength, float fTargetLength)
{
  NS_TEST_FLOAT(atoB.ConvertSourceLength(fSourceLength), fTargetLength, nsMath::DefaultEpsilon<float>());
  NS_TEST_FLOAT(atoB.ConvertTargetLength(fTargetLength), fSourceLength, nsMath::DefaultEpsilon<float>());

  NS_TEST_FLOAT(btoA.ConvertTargetLength(fSourceLength), fTargetLength, nsMath::DefaultEpsilon<float>());
  NS_TEST_FLOAT(btoA.ConvertSourceLength(fTargetLength), fSourceLength, nsMath::DefaultEpsilon<float>());
}

void TestPosition(
  const nsCoordinateSystemConversion& atoB, const nsCoordinateSystemConversion& btoA, const nsVec3& vSourcePos, const nsVec3& vTargetPos)
{
  TestLength(atoB, btoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  NS_TEST_VEC3(atoB.ConvertSourcePosition(vSourcePos), vTargetPos, nsMath::DefaultEpsilon<float>());
  NS_TEST_VEC3(atoB.ConvertTargetPosition(vTargetPos), vSourcePos, nsMath::DefaultEpsilon<float>());
  NS_TEST_VEC3(btoA.ConvertSourcePosition(vTargetPos), vSourcePos, nsMath::DefaultEpsilon<float>());
  NS_TEST_VEC3(btoA.ConvertTargetPosition(vSourcePos), vTargetPos, nsMath::DefaultEpsilon<float>());
}

void TestRotation(const nsCoordinateSystemConversion& atoB, const nsCoordinateSystemConversion& btoA, const nsVec3& vSourceStartDir,
  const nsVec3& vSourceEndDir, const nsQuat& qSourceRot, const nsVec3& vTargetStartDir, const nsVec3& vTargetEndDir, const nsQuat& qTargetRot)
{
  TestPosition(atoB, btoA, vSourceStartDir, vTargetStartDir);
  TestPosition(atoB, btoA, vSourceEndDir, vTargetEndDir);

  NS_TEST_BOOL(atoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, nsMath::DefaultEpsilon<float>()));
  NS_TEST_BOOL(atoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, nsMath::DefaultEpsilon<float>()));
  NS_TEST_BOOL(btoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, nsMath::DefaultEpsilon<float>()));
  NS_TEST_BOOL(btoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, nsMath::DefaultEpsilon<float>()));

  NS_TEST_VEC3(qSourceRot * vSourceStartDir, vSourceEndDir, nsMath::DefaultEpsilon<float>());
  NS_TEST_VEC3(qTargetRot * vTargetStartDir, vTargetEndDir, nsMath::DefaultEpsilon<float>());
}

nsQuat FromAxisAndAngle(const nsVec3& vAxis, nsAngle angle)
{
  nsQuat q = nsQuat::MakeFromAxisAndAngle(vAxis.GetNormalized(), angle);
  return q;
}

bool IsRightHanded(const nsCoordinateSystem& cs)
{
  nsVec3 vF = cs.m_vUpDir.CrossRH(cs.m_vRightDir);

  return vF.Dot(cs.m_vForwardDir) > 0;
}

void TestCoordinateSystemConversion(const nsCoordinateSystem& a, const nsCoordinateSystem& b)
{
  const bool bAisRH = IsRightHanded(a);
  const bool bBisRH = IsRightHanded(b);
  const nsAngle A_CWRot = bAisRH ? nsAngle::MakeFromDegree(-90.0f) : nsAngle::MakeFromDegree(90.0f);
  const nsAngle B_CWRot = bBisRH ? nsAngle::MakeFromDegree(-90.0f) : nsAngle::MakeFromDegree(90.0f);

  nsCoordinateSystemConversion AtoB;
  AtoB.SetConversion(a, b);

  nsCoordinateSystemConversion BtoA;
  BtoA.SetConversion(b, a);

  TestPosition(AtoB, BtoA, a.m_vForwardDir, b.m_vForwardDir);
  TestPosition(AtoB, BtoA, a.m_vRightDir, b.m_vRightDir);
  TestPosition(AtoB, BtoA, a.m_vUpDir, b.m_vUpDir);

  TestRotation(AtoB, BtoA, a.m_vForwardDir, a.m_vRightDir, FromAxisAndAngle(a.m_vUpDir, A_CWRot), b.m_vForwardDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vForwardDir, FromAxisAndAngle(a.m_vRightDir, A_CWRot), b.m_vUpDir, b.m_vForwardDir,
    FromAxisAndAngle(b.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vRightDir, FromAxisAndAngle(a.m_vForwardDir, -A_CWRot), b.m_vUpDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vForwardDir, -B_CWRot));
}


NS_CREATE_SIMPLE_TEST(World, CoordinateSystem)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "NS / OpenXR")
  {
    nsCoordinateSystem nsCoordSysLH;
    nsCoordSysLH.m_vForwardDir = nsVec3(1.0f, 0.0f, 0.0f);
    nsCoordSysLH.m_vRightDir = nsVec3(0.0f, 1.0f, 0.0f);
    nsCoordSysLH.m_vUpDir = nsVec3(0.0f, 0.0f, 1.0f);

    nsCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = nsVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = nsVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = nsVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(nsCoordSysLH, openXrRH);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Scaled NS / Scaled OpenXR")
  {
    nsCoordinateSystem nsCoordSysLH;
    nsCoordSysLH.m_vForwardDir = nsVec3(0.1f, 0.0f, 0.0f);
    nsCoordSysLH.m_vRightDir = nsVec3(0.0f, 0.1f, 0.0f);
    nsCoordSysLH.m_vUpDir = nsVec3(0.0f, 0.0f, 0.1f);

    nsCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = nsVec3(0.0f, 0.0f, -20.0f);
    openXrRH.m_vRightDir = nsVec3(20.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = nsVec3(0.0f, 20.0f, 0.0f);

    TestCoordinateSystemConversion(nsCoordSysLH, openXrRH);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "NS / Flipped NS")
  {
    nsCoordinateSystem nsCoordSysLH;
    nsCoordSysLH.m_vForwardDir = nsVec3(1.0f, 0.0f, 0.0f);
    nsCoordSysLH.m_vRightDir = nsVec3(0.0f, 1.0f, 0.0f);
    nsCoordSysLH.m_vUpDir = nsVec3(0.0f, 0.0f, 1.0f);

    nsCoordinateSystem nsCoordSysFlippedLH;
    nsCoordSysFlippedLH.m_vForwardDir = nsVec3(-1.0f, 0.0f, 0.0f);
    nsCoordSysFlippedLH.m_vRightDir = nsVec3(0.0f, -1.0f, 0.0f);
    nsCoordSysFlippedLH.m_vUpDir = nsVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(nsCoordSysLH, nsCoordSysFlippedLH);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "OpenXR / Flipped OpenXR")
  {
    nsCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = nsVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = nsVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = nsVec3(0.0f, 1.0f, 0.0f);

    nsCoordinateSystem openXrFlippedRH;
    openXrFlippedRH.m_vForwardDir = nsVec3(0.0f, 0.0f, 1.0f);
    openXrFlippedRH.m_vRightDir = nsVec3(-1.0f, 0.0f, 0.0f);
    openXrFlippedRH.m_vUpDir = nsVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(openXrRH, openXrFlippedRH);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Identity")
  {
    nsCoordinateSystem nsCoordSysLH;
    nsCoordSysLH.m_vForwardDir = nsVec3(1.0f, 0.0f, 0.0f);
    nsCoordSysLH.m_vRightDir = nsVec3(0.0f, 1.0f, 0.0f);
    nsCoordSysLH.m_vUpDir = nsVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(nsCoordSysLH, nsCoordSysLH);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Default Constructed")
  {
    nsCoordinateSystem nsCoordSysLH;
    nsCoordSysLH.m_vForwardDir = nsVec3(1.0f, 0.0f, 0.0f);
    nsCoordSysLH.m_vRightDir = nsVec3(0.0f, 1.0f, 0.0f);
    nsCoordSysLH.m_vUpDir = nsVec3(0.0f, 0.0f, 1.0f);

    const nsAngle rot = nsAngle::MakeFromDegree(90.0f);

    nsCoordinateSystemConversion defaultConstucted;

    TestPosition(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vForwardDir, nsCoordSysLH.m_vForwardDir);
    TestPosition(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vRightDir, nsCoordSysLH.m_vRightDir);
    TestPosition(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vUpDir, nsCoordSysLH.m_vUpDir);

    TestRotation(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vForwardDir, nsCoordSysLH.m_vRightDir,
      FromAxisAndAngle(nsCoordSysLH.m_vUpDir, rot), nsCoordSysLH.m_vForwardDir, nsCoordSysLH.m_vRightDir,
      FromAxisAndAngle(nsCoordSysLH.m_vUpDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vUpDir, nsCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(nsCoordSysLH.m_vRightDir, rot), nsCoordSysLH.m_vUpDir, nsCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(nsCoordSysLH.m_vRightDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, nsCoordSysLH.m_vUpDir, nsCoordSysLH.m_vRightDir,
      FromAxisAndAngle(nsCoordSysLH.m_vForwardDir, -rot), nsCoordSysLH.m_vUpDir, nsCoordSysLH.m_vRightDir,
      FromAxisAndAngle(nsCoordSysLH.m_vForwardDir, -rot));
  }
}
