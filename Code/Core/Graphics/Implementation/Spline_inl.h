
NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetPosition(const nsSimdVec4f& vPos)
{
  m_vPos = vPos;
}

NS_ALWAYS_INLINE nsSplineTangentMode::Enum nsSpline::ControlPoint::GetTangentModeIn() const
{
  float w = m_vPosTangentIn.w();
  return static_cast<nsSplineTangentMode::Enum>(w);
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetTangentModeIn(nsSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentIn.SetW(w);
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetTangentIn(const nsSimdVec4f& vTangent, nsSplineTangentMode::Enum mode /*= nsSplineTangentMode::Custom*/)
{
  m_vPosTangentIn = vTangent;
  SetTangentModeIn(mode);
}

NS_ALWAYS_INLINE nsSplineTangentMode::Enum nsSpline::ControlPoint::GetTangentModeOut() const
{
  float w = m_vPosTangentOut.w();
  return static_cast<nsSplineTangentMode::Enum>(w);
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetTangentModeOut(nsSplineTangentMode::Enum mode)
{
  float w = static_cast<float>(mode);
  m_vPosTangentOut.SetW(w);
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetTangentOut(const nsSimdVec4f& vTangent, nsSplineTangentMode::Enum mode /*= nsSplineTangentMode::Custom*/)
{
  m_vPosTangentOut = vTangent;
  SetTangentModeOut(mode);
}

NS_ALWAYS_INLINE nsAngle nsSpline::ControlPoint::GetRoll() const
{
  return nsAngle::MakeFromRadian(m_vUpDirAndRoll.w());
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetRoll(nsAngle roll)
{
  m_vUpDirAndRoll.SetW(roll);
}

NS_ALWAYS_INLINE void nsSpline::ControlPoint::SetScale(const nsSimdVec4f& vScale)
{
  m_vScale = vScale;
  m_vScaleTangentIn.SetZero();
  m_vScaleTangentOut.SetZero();
}

//////////////////////////////////////////////////////////////////////////

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluatePosition(float fT) const
{
  nsUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluatePosition(uiCp0, fT);
}

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluatePosition(nsUInt32 uiCp0, const nsSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return nsSimdVec4f::MakeZero();

  const nsUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluatePosition(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluateDerivative(float fT) const
{
  nsUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  return EvaluateDerivative(uiCp0, fT);
}

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluateDerivative(nsUInt32 uiCp0, const nsSimdFloat& fT) const
{
  if (m_ControlPoints.IsEmpty())
    return nsSimdVec4f::MakeZero();

  const nsUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateDerivative(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluateUpDirection(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return nsSimdVec4f::MakeZero();

  nsUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const nsUInt32 uiCp1 = GetCp1Index(uiCp0);

  nsSimdVec4f forwardDir, rightDir, upDir;
  EvaluateRotation(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT, forwardDir, rightDir, upDir);

  return upDir;
}

NS_FORCE_INLINE nsSimdVec4f nsSpline::EvaluateScale(float fT) const
{
  if (m_ControlPoints.IsEmpty())
    return nsSimdVec4f::MakeZero();

  nsUInt32 uiCp0;
  fT = ClampAndSplitT(fT, uiCp0);

  const nsUInt32 uiCp1 = GetCp1Index(uiCp0);

  return EvaluateScale(m_ControlPoints[uiCp0], m_ControlPoints[uiCp1], fT);
}

NS_ALWAYS_INLINE float nsSpline::ClampAndSplitT(float fT, nsUInt32& out_uiIndex) const
{
  float fNumPoints = static_cast<float>(m_ControlPoints.GetCount());
  if (m_bClosed)
  {
    fT = (fT < 0.0f || fT >= fNumPoints) ? 0.0f : fT;
  }
  else
  {
    fT = nsMath::Clamp(fT, 0.0f, fNumPoints - 1.0f);
  }

  const float fIndex = nsMath::Floor(fT);
  out_uiIndex = static_cast<nsUInt32>(fIndex);
  return fT - fIndex;
}

NS_ALWAYS_INLINE nsUInt32 nsSpline::GetCp1Index(nsUInt32 uiCp0) const
{
  return (uiCp0 + 1 < m_ControlPoints.GetCount()) ? uiCp0 + 1 : 0;
}

NS_ALWAYS_INLINE nsSimdVec4f nsSpline::EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const
{
  return nsMath::EvaluateBnsierCurve(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

NS_ALWAYS_INLINE nsSimdVec4f nsSpline::EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const
{
  return nsMath::EvaluateBnsierCurveDerivative(fT, cp0.m_vPos, cp0.m_vPos + cp0.m_vPosTangentOut, cp1.m_vPos + cp1.m_vPosTangentIn, cp1.m_vPos);
}

NS_ALWAYS_INLINE void nsSpline::EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT, nsSimdVec4f& out_forwardDir, nsSimdVec4f& out_rightDir, nsSimdVec4f& out_upDir) const
{
  nsSimdVec4f upDir = nsMath::EvaluateBnsierCurve(fT, cp0.m_vUpDirAndRoll, cp0.m_vUpDirAndRoll + cp0.m_vUpDirTangentOut, cp1.m_vUpDirAndRoll + cp1.m_vUpDirTangentIn, cp1.m_vUpDirAndRoll);

  out_forwardDir = EvaluateDerivative(cp0, cp1, fT);
  out_forwardDir.NormalizeIfNotZero<3>(nsSimdVec4f(1, 0, 0));

  out_rightDir = upDir.CrossRH(out_forwardDir).GetNormalized<3>();
  out_upDir = out_forwardDir.CrossRH(out_rightDir).GetNormalized<3>();
}

NS_ALWAYS_INLINE nsSimdVec4f nsSpline::EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const nsSimdFloat& fT) const
{
  return nsMath::EvaluateBnsierCurve(fT, cp0.m_vScale, cp0.m_vScale + cp0.m_vScaleTangentOut, cp1.m_vScale + cp1.m_vScaleTangentIn, cp1.m_vScale);
}
