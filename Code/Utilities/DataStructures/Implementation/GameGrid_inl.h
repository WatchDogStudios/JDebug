#pragma once

template <class CellData>
nsGameGrid<CellData>::nsGameGrid()
{
  m_uiGridSizeX = 0;
  m_uiGridSizeY = 0;

  m_mRotateToWorldspace.SetIdentity();
  m_mRotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vLocalSpaceCellSize.Set(1.0f);
  m_vInverseLocalSpaceCellSize.Set(1.0f);
}

template <class CellData>
void nsGameGrid<CellData>::CreateGrid(nsUInt16 uiSizeX, nsUInt16 uiSizeY)
{
  m_Cells.Clear();

  m_uiGridSizeX = uiSizeX;
  m_uiGridSizeY = uiSizeY;

  m_Cells.SetCount(m_uiGridSizeX * m_uiGridSizeY);
}

template <class CellData>
void nsGameGrid<CellData>::SetWorldSpaceDimensions(const nsVec3& vLowerLeftCorner, const nsVec3& vCellSize, Orientation ori)
{
  nsMat3 mRot;

  switch (ori)
  {
    case InPlaneXY:
      mRot.SetIdentity();
      break;
    case InPlaneXZ:
      mRot = nsMat3::MakeAxisRotation(nsVec3(1, 0, 0), nsAngle::MakeFromDegree(90.0f));
      break;
    case InPlaneXminusZ:
      mRot = nsMat3::MakeAxisRotation(nsVec3(1, 0, 0), nsAngle::MakeFromDegree(-90.0f));
      break;
  }

  SetWorldSpaceDimensions(vLowerLeftCorner, vCellSize, mRot);
}

template <class CellData>
void nsGameGrid<CellData>::SetWorldSpaceDimensions(const nsVec3& vLowerLeftCorner, const nsVec3& vCellSize, const nsMat3& mRotation)
{
  m_vWorldSpaceOrigin = vLowerLeftCorner;
  m_vLocalSpaceCellSize = vCellSize;
  m_vInverseLocalSpaceCellSize = nsVec3(1.0f).CompDiv(vCellSize);

  m_mRotateToWorldspace = mRotation;
  m_mRotateToGridspace = mRotation.GetInverse();
}

template <class CellData>
nsVec2I32 nsGameGrid<CellData>::GetCellAtWorldPosition(const nsVec3& vWorldSpacePos) const
{
  const nsVec3 vCell = (m_mRotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin)).CompMul(m_vInverseLocalSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return nsVec2I32((nsInt32)nsMath::Floor(vCell.x), (nsInt32)nsMath::Floor(vCell.y));
}

template <class CellData>
nsVec3 nsGameGrid<CellData>::GetCellWorldSpaceOrigin(const nsVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(vCoord);
}

template <class CellData>
nsVec3 nsGameGrid<CellData>::GetCellLocalSpaceOrigin(const nsVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(nsVec3((float)vCoord.x, (float)vCoord.y, 0.0f));
}

template <class CellData>
nsVec3 nsGameGrid<CellData>::GetCellWorldSpaceCenter(const nsVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord);
}

template <class CellData>
nsVec3 nsGameGrid<CellData>::GetCellLocalSpaceCenter(const nsVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(nsVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, 0.5f));
}

template <class CellData>
bool nsGameGrid<CellData>::IsValidCellCoordinate(const nsVec2I32& vCoord) const
{
  return (vCoord.x >= 0 && vCoord.x < m_uiGridSizeX && vCoord.y >= 0 && vCoord.y < m_uiGridSizeY);
}

template <class CellData>
bool nsGameGrid<CellData>::PickCell(const nsVec3& vRayStartPos, const nsVec3& vRayDirNorm, nsVec2I32* out_pCellCoord, nsVec3* out_pIntersection) const
{
  nsPlane p;
  p = nsPlane::MakeFromNormalAndPoint(m_mRotateToWorldspace * nsVec3(0, 0, -1), m_vWorldSpaceOrigin);

  nsVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_pIntersection)
    *out_pIntersection = vPos;

  if (out_pCellCoord)
    *out_pCellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template <class CellData>
nsBoundingBox nsGameGrid<CellData>::GetWorldBoundingBox() const
{
  nsVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  vGridBox = m_mRotateToWorldspace * m_vLocalSpaceCellSize.CompMul(vGridBox);

  return nsBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + vGridBox);
}

template <class CellData>
bool nsGameGrid<CellData>::GetRayIntersection(const nsVec3& vRayStartWorldSpace, const nsVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
  float& out_fIntersection, nsVec2I32& out_vCellCoord) const
{
  const nsVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const nsVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  nsVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  const nsBoundingBox localBox(nsVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  const nsVec3 vEnterPos = vRayStart + vRayDir * out_fIntersection;

  const nsVec3 vCell = vEnterPos.CompMul(m_vInverseLocalSpaceCellSize);

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  out_vCellCoord = nsVec2I32((nsInt32)nsMath::Floor(vCell.x), (nsInt32)nsMath::Floor(vCell.y));
  out_vCellCoord.x = nsMath::Clamp(out_vCellCoord.x, 0, m_uiGridSizeX - 1);
  out_vCellCoord.y = nsMath::Clamp(out_vCellCoord.y, 0, m_uiGridSizeY - 1);

  return true;
}

template <class CellData>
bool nsGameGrid<CellData>::GetRayIntersectionExpandedBBox(const nsVec3& vRayStartWorldSpace, const nsVec3& vRayDirNormalizedWorldSpace,
  float fMaxLength, float& out_fIntersection, const nsVec3& vExpandBBoxByThis) const
{
  const nsVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const nsVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  nsVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  nsBoundingBox localBox(nsVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));
  localBox.Grow(vExpandBBoxByThis);

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  return true;
}

template <class CellData>
void nsGameGrid<CellData>::ComputeWorldSpaceCorners(nsVec3* pCorners) const
{
  pCorners[0] = m_vWorldSpaceOrigin;
  pCorners[1] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * nsVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, 0, 0);
  pCorners[2] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * nsVec3(0, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
  pCorners[3] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * nsVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
}


template <class CellData>
nsResult nsGameGrid<CellData>::Serialize(nsStreamWriter& ref_stream) const
{
  auto& stream = ref_stream;

  stream.WriteVersion(1);

  stream << m_uiGridSizeX;
  stream << m_uiGridSizeY;
  stream << m_mRotateToWorldspace;
  stream << m_mRotateToGridspace;
  stream << m_vWorldSpaceOrigin;
  stream << m_vLocalSpaceCellSize;
  stream << m_vInverseLocalSpaceCellSize;
  NS_SUCCEED_OR_RETURN(stream.WriteArray(m_Cells));

  return NS_SUCCESS;
}

template <class CellData>
nsResult nsGameGrid<CellData>::Deserialize(nsStreamReader& ref_stream)
{
  auto& stream = ref_stream;

  const nsTypeVersion version = stream.ReadVersion(1);
  NS_IGNORE_UNUSED(version);

  stream >> m_uiGridSizeX;
  stream >> m_uiGridSizeY;
  stream >> m_mRotateToWorldspace;
  stream >> m_mRotateToGridspace;
  stream >> m_vWorldSpaceOrigin;
  stream >> m_vLocalSpaceCellSize;
  stream >> m_vInverseLocalSpaceCellSize;
  NS_SUCCEED_OR_RETURN(stream.ReadArray(m_Cells));

  return NS_SUCCESS;
}
