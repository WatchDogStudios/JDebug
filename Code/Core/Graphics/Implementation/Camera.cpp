#include <Core/CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Utilities/GraphicsUtils.h>

class RemapCoordinateSystemProvider : public nsCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider()
    : nsCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystem(const nsVec3& vGlobalPosition, nsCoordinateSystem& out_coordinateSystem) const override
  {
    NS_IGNORE_UNUSED(vGlobalPosition);

    out_coordinateSystem.m_vForwardDir = nsBasisAxis::GetBasisVector(m_ForwardAxis);
    out_coordinateSystem.m_vRightDir = nsBasisAxis::GetBasisVector(m_RightAxis);
    out_coordinateSystem.m_vUpDir = nsBasisAxis::GetBasisVector(m_UpAxis);
  }

  nsBasisAxis::Enum m_ForwardAxis = nsBasisAxis::PositiveX;
  nsBasisAxis::Enum m_RightAxis = nsBasisAxis::PositiveY;
  nsBasisAxis::Enum m_UpAxis = nsBasisAxis::PositiveZ;
};

// clang-format off
NS_BEGIN_STATIC_REFLECTED_ENUM(nsCameraMode, 1)
  NS_ENUM_CONSTANT(nsCameraMode::PerspectiveFixedFovX),
  NS_ENUM_CONSTANT(nsCameraMode::PerspectiveFixedFovY),
  NS_ENUM_CONSTANT(nsCameraMode::OrthoFixedWidth),
  NS_ENUM_CONSTANT(nsCameraMode::OrthoFixedHeight),
NS_END_STATIC_REFLECTED_ENUM;
// clang-format on

nsCamera::nsCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(nsBasisAxis::PositiveX, nsBasisAxis::PositiveY, nsBasisAxis::PositiveZ);
}

void nsCamera::SetCoordinateSystem(nsBasisAxis::Enum forwardAxis, nsBasisAxis::Enum rightAxis, nsBasisAxis::Enum axis)
{
  auto provider = NS_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis = rightAxis;
  provider->m_UpAxis = axis;

  m_pCoordinateSystem = provider;
}

void nsCamera::SetCoordinateSystem(const nsSharedPtr<nsCoordinateSystemProvider>& pProvider)
{
  m_pCoordinateSystem = pProvider;
}

nsVec3 nsCamera::GetPosition(nsCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

nsVec3 nsCamera::GetDirForwards(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

nsVec3 nsCamera::GetDirUp(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

nsVec3 nsCamera::GetDirRight(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

nsVec3 nsCamera::InternalGetPosition(nsCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

nsVec3 nsCamera::InternalGetDirForwards(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return decFwd;
}

nsVec3 nsCamera::InternalGetDirUp(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return decUp;
}

nsVec3 nsCamera::InternalGetDirRight(nsCameraEye eye) const
{
  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  return -decRight;
}

nsVec3 nsCamera::MapExternalToInternal(const nsVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    nsCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    nsMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

nsVec3 nsCamera::MapInternalToExternal(const nsVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    nsCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    nsMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

nsAngle nsCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == nsCameraMode::PerspectiveFixedFovX)
    return nsAngle::MakeFromDegree(m_fFovOrDim);

  if (m_Mode == nsCameraMode::PerspectiveFixedFovY)
    return nsMath::ATan(nsMath::Tan(nsAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == nsCameraMode::Stereo)
    return nsAngle::MakeFromDegree(90);

  NS_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return nsAngle();
}

nsAngle nsCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == nsCameraMode::PerspectiveFixedFovX)
    return nsMath::ATan(nsMath::Tan(nsAngle::MakeFromDegree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == nsCameraMode::PerspectiveFixedFovY)
    return nsAngle::MakeFromDegree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == nsCameraMode::Stereo)
    return nsAngle::MakeFromDegree(90);

  NS_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return nsAngle();
}


float nsCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == nsCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == nsCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  NS_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float nsCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == nsCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == nsCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  NS_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void nsCamera::SetCameraMode(nsCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void nsCamera::SetStereoProjection(const nsMat4& mProjectionLeftEye, const nsMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(nsCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(nsCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void nsCamera::LookAt(const nsVec3& vCameraPos0, const nsVec3& vTargetPos0, const nsVec3& vUp0)
{
  const nsVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const nsVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const nsVec3 vUp = MapExternalToInternal(vUp0);

  if (m_Mode == nsCameraMode::Stereo)
  {
    NS_REPORT_FAILURE("nsCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0] = nsGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, nsHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged();
}

void nsCamera::SetViewMatrix(const nsMat4& mLookAtMatrix, nsCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  nsVec3 decFwd, decRight, decUp;
  nsGraphicsUtils::DecomposeViewMatrix(
    m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], nsHandedness::LeftHanded);

  if (m_Mode != nsCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx] = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged();
}

void nsCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, nsMat4& out_mProjectionMatrix, nsCameraEye eye, nsClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case nsCameraMode::PerspectiveFixedFovX:
      out_mProjectionMatrix = nsGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(nsAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, nsClipSpaceYMode::Regular, nsHandedness::LeftHanded);
      break;

    case nsCameraMode::PerspectiveFixedFovY:
      out_mProjectionMatrix = nsGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(nsAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, nsClipSpaceYMode::Regular, nsHandedness::LeftHanded);
      break;

    case nsCameraMode::OrthoFixedWidth:
      out_mProjectionMatrix = nsGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane,
        m_fFarPlane, depthRange, nsClipSpaceYMode::Regular, nsHandedness::LeftHanded);
      break;

    case nsCameraMode::OrthoFixedHeight:
      out_mProjectionMatrix = nsGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane,
        m_fFarPlane, depthRange, nsClipSpaceYMode::Regular, nsHandedness::LeftHanded);
      break;

    case nsCameraMode::Stereo:
      if (nsMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, nsMath::LargeEpsilon<float>()))
        out_mProjectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_mProjectionMatrix = nsGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(nsAngle::MakeFromDegree(m_fFovOrDim), fAspectRatioWidthDivHeight,
          m_fNearPlane, m_fFarPlane, depthRange, nsClipSpaceYMode::Regular, nsHandedness::LeftHanded);
      }
      break;

    default:
      NS_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void nsCamera::CameraSettingsChanged()
{
  NS_ASSERT_DEV(m_Mode != nsCameraMode::None, "Invalid Camera Mode.");
  NS_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  NS_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void nsCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - nsVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], nsHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged();
}

void nsCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  nsVec3 vMove(fForward, fRight, fUp);

  nsVec3 decFwd, decRight, decUp, decPos;
  nsGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], nsHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = nsGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, nsHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged();
}

void nsCamera::ClampRotationAngles(bool bLocalSpace, nsAngle& forwardAxis, nsAngle& rightAxis, nsAngle& upAxis)
{
  NS_IGNORE_UNUSED(forwardAxis);
  NS_IGNORE_UNUSED(upAxis);

  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = InternalGetDirForwards().Dot(nsVec3(0, 0, -1));
      const nsAngle fCurAngle = nsMath::ACos(fDot) - nsAngle::MakeFromDegree(90.0f);
      const nsAngle fNewAngle = fCurAngle + rightAxis;

      const nsAngle fAllowedAngle = nsMath::Clamp(fNewAngle, nsAngle::MakeFromDegree(-85.0f), nsAngle::MakeFromDegree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void nsCamera::RotateLocally(nsAngle forwardAxis, nsAngle rightAxis, nsAngle axis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, axis);

  nsVec3 vDirForwards = InternalGetDirForwards();
  nsVec3 vDirUp = InternalGetDirUp();
  nsVec3 vDirRight = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    nsMat3 m = nsMat3::MakeAxisRotation(vDirForwards, forwardAxis);

    vDirUp = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    nsMat3 m = nsMat3::MakeAxisRotation(vDirRight, rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    nsMat3 m = nsMat3::MakeAxisRotation(vDirUp, axis);

    vDirRight = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using nsGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = nsGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, nsHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged();
}

void nsCamera::RotateGlobally(nsAngle forwardAxis, nsAngle rightAxis, nsAngle axis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, axis);

  nsVec3 vDirForwards = InternalGetDirForwards();
  nsVec3 vDirUp = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    nsMat3 m;
    m = nsMat3::MakeRotationX(forwardAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    nsMat3 m;
    m = nsMat3::MakeRotationY(rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    nsMat3 m;
    m = nsMat3::MakeRotationZ(axis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using nsGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = nsGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, nsHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged();
}



NS_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);
