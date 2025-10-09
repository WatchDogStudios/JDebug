#pragma once

inline nsVec3 nsCamera::GetCenterPosition() const
{
  if (m_Mode == nsCameraMode::Stereo)
    return (GetPosition(nsCameraEye::Left) + GetPosition(nsCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline nsVec3 nsCamera::GetCenterDirForwards() const
{
  if (m_Mode == nsCameraMode::Stereo)
    return (GetDirForwards(nsCameraEye::Left) + GetDirForwards(nsCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline nsVec3 nsCamera::GetCenterDirUp() const
{
  if (m_Mode == nsCameraMode::Stereo)
    return (GetDirUp(nsCameraEye::Left) + GetDirUp(nsCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline nsVec3 nsCamera::GetCenterDirRight() const
{
  if (m_Mode == nsCameraMode::Stereo)
    return (GetDirRight(nsCameraEye::Left) + GetDirRight(nsCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

NS_ALWAYS_INLINE float nsCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

NS_ALWAYS_INLINE float nsCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

NS_ALWAYS_INLINE float nsCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

NS_ALWAYS_INLINE nsCameraMode::Enum nsCamera::GetCameraMode() const
{
  return m_Mode;
}

NS_ALWAYS_INLINE bool nsCamera::IsPerspective() const
{
  return m_Mode == nsCameraMode::PerspectiveFixedFovX || m_Mode == nsCameraMode::PerspectiveFixedFovY ||
         m_Mode == nsCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

NS_ALWAYS_INLINE bool nsCamera::IsOrthographic() const
{
  return m_Mode == nsCameraMode::OrthoFixedWidth || m_Mode == nsCameraMode::OrthoFixedHeight;
}

NS_ALWAYS_INLINE bool nsCamera::IsStereoscopic() const
{
  return m_Mode == nsCameraMode::Stereo;
}

NS_ALWAYS_INLINE float nsCamera::GetExposure() const
{
  return m_fExposure;
}

NS_ALWAYS_INLINE void nsCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

NS_ALWAYS_INLINE const nsMat4& nsCamera::GetViewMatrix(nsCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}
