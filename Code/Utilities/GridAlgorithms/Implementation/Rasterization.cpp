#include <Utilities/UtilitiesPCH.h>

#include <Utilities/GridAlgorithms/Rasterization.h>

nsRasterizationResult::Enum ns2DGridUtils::ComputePointsOnLine(
  nsInt32 iStartX, nsInt32 iStartY, nsInt32 iEndX, nsInt32 iEndY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  // Implements Bresenham's line algorithm:
  // http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

  nsInt32 dx = nsMath::Abs(iEndX - iStartX);
  nsInt32 dy = nsMath::Abs(iEndY - iStartY);

  nsInt32 sx = (iStartX < iEndX) ? 1 : -1;
  nsInt32 sy = (iStartY < iEndY) ? 1 : -1;

  nsInt32 err = dx - dy;

  while (true)
  {
    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return nsRasterizationResult::Finished;

    nsInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}

nsRasterizationResult::Enum ns2DGridUtils::ComputePointsOnLineConservative(nsInt32 iStartX, nsInt32 iStartY, nsInt32 iEndX, nsInt32 iEndY,
  NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, bool bVisitBothNeighbors /* = false */)
{
  nsInt32 dx = nsMath::Abs(iEndX - iStartX);
  nsInt32 dy = nsMath::Abs(iEndY - iStartY);

  nsInt32 sx = (iStartX < iEndX) ? 1 : -1;
  nsInt32 sy = (iStartY < iEndY) ? 1 : -1;

  nsInt32 err = dx - dy;

  nsInt32 iLastX = iStartX;
  nsInt32 iLastY = iStartY;

  while (true)
  {
    // if this is going to be a diagonal step, make sure to insert horizontal/vertical steps

    if ((nsMath::Abs(iLastX - iStartX) + nsMath::Abs(iLastY - iStartY)) == 2)
    {
      // This part is the difference to the non-conservative line algorithm

      if (callback(iLastX, iStartY, pPassThrough) == nsCallbackResult::Continue)
      {
        // first one succeeded, going to continue

        // if this is true, the user still wants a callback for the alternative, even though it does not change the outcome anymore
        if (bVisitBothNeighbors)
          callback(iStartX, iLastY, pPassThrough);
      }
      else
      {
        // first one failed, try the second
        if (callback(iStartX, iLastY, pPassThrough) == nsCallbackResult::Stop)
          return nsRasterizationResult::Aborted;
      }
    }

    iLastX = iStartX;
    iLastY = iStartY;

    // The user callback can stop the algorithm at any point, if no further points on the line are required
    if (callback(iStartX, iStartY, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;

    if ((iStartX == iEndX) && (iStartY == iEndY))
      return nsRasterizationResult::Finished;

    nsInt32 e2 = 2 * err;

    if (e2 > -dy)
    {
      err = err - dy;
      iStartX = iStartX + sx;
    }

    if (e2 < dx)
    {
      err = err + dx;
      iStartY = iStartY + sy;
    }
  }
}


nsRasterizationResult::Enum ns2DGridUtils::ComputePointsOnCircle(
  nsInt32 iStartX, nsInt32 iStartY, nsUInt32 uiRadius, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  int f = 1 - uiRadius;
  int ddF_x = 1;
  int ddF_y = -2 * uiRadius;
  int x = 0;
  int y = uiRadius;

  // report the four extremes
  if (callback(iStartX, iStartY + uiRadius, pPassThrough) == nsCallbackResult::Stop)
    return nsRasterizationResult::Aborted;
  if (callback(iStartX, iStartY - uiRadius, pPassThrough) == nsCallbackResult::Stop)
    return nsRasterizationResult::Aborted;
  if (callback(iStartX + uiRadius, iStartY, pPassThrough) == nsCallbackResult::Stop)
    return nsRasterizationResult::Aborted;
  if (callback(iStartX - uiRadius, iStartY, pPassThrough) == nsCallbackResult::Stop)
    return nsRasterizationResult::Aborted;

  // the loop iterates over an eighth of the circle (a 45 degree segment) and then mirrors each point 8 times to fill the entire circle
  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    if (callback(iStartX + x, iStartY + y, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY + y, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX + x, iStartY - y, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX - x, iStartY - y, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY + x, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY + x, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX + y, iStartY - x, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
    if (callback(iStartX - y, iStartY - x, pPassThrough) == nsCallbackResult::Stop)
      return nsRasterizationResult::Aborted;
  }

  return nsRasterizationResult::Finished;
}

nsUInt32 ns2DGridUtils::FloodFill(nsInt32 iStartX, nsInt32 iStartY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */,
  nsDeque<nsVec2I32>* pTempArray /* = nullptr */)
{
  nsUInt32 uiFilled = 0;

  nsDeque<nsVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(nsVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    nsVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == nsCallbackResult::Continue)
    {
      ++uiFilled;

      // put the four neighbors into the queue
      pTempArray->PushBack(nsVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(nsVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(nsVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(nsVec2I32(v.x, v.y + 1));
    }
  }

  return uiFilled;
}

nsUInt32 ns2DGridUtils::FloodFillDiag(nsInt32 iStartX, nsInt32 iStartY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /*= nullptr*/,
  nsDeque<nsVec2I32>* pTempArray /*= nullptr*/)
{
  nsUInt32 uiFilled = 0;

  nsDeque<nsVec2I32> FallbackQueue;

  if (pTempArray == nullptr)
    pTempArray = &FallbackQueue;

  pTempArray->Clear();
  pTempArray->PushBack(nsVec2I32(iStartX, iStartY));

  while (!pTempArray->IsEmpty())
  {
    nsVec2I32 v = pTempArray->PeekBack();
    pTempArray->PopBack();

    if (callback(v.x, v.y, pPassThrough) == nsCallbackResult::Continue)
    {
      ++uiFilled;

      // put the eight neighbors into the queue
      pTempArray->PushBack(nsVec2I32(v.x - 1, v.y));
      pTempArray->PushBack(nsVec2I32(v.x + 1, v.y));
      pTempArray->PushBack(nsVec2I32(v.x, v.y - 1));
      pTempArray->PushBack(nsVec2I32(v.x, v.y + 1));

      pTempArray->PushBack(nsVec2I32(v.x - 1, v.y - 1));
      pTempArray->PushBack(nsVec2I32(v.x + 1, v.y - 1));
      pTempArray->PushBack(nsVec2I32(v.x + 1, v.y + 1));
      pTempArray->PushBack(nsVec2I32(v.x - 1, v.y + 1));
    }
  }

  return uiFilled;
}

// Lookup table that describes the shape of the circle
// When rasterizing circles with few pixels algorithms usually don't give nice shapes
// so this lookup table is handcrafted for better results
static const nsUInt8 OverlapCircle[15][15] = {{9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 5, 4, 3, 1, 0, 1, 3, 4, 5, 6, 7, 8},
  {8, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, {8, 7, 6, 6, 5, 4, 3, 3, 3, 4, 5, 6, 6, 7, 8}, {9, 8, 7, 6, 6, 5, 4, 4, 4, 5, 6, 6, 7, 8, 9},
  {9, 8, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 8, 9}, {9, 9, 8, 8, 7, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9}, {9, 9, 9, 8, 8, 7, 7, 7, 7, 7, 8, 8, 9, 9, 9},
  {9, 9, 9, 9, 9, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9}};

static const nsInt32 CircleCenter = 7;
static const nsUInt8 CircleAreaMin[9] = {7, 6, 6, 5, 4, 3, 2, 1, 0};
static const nsUInt8 CircleAreaMax[9] = {7, 8, 8, 9, 10, 11, 12, 13, 14};

nsRasterizationResult::Enum ns2DGridUtils::RasterizeBlob(
  nsInt32 iPosX, nsInt32 iPosY, nsBlobType type, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const nsUInt8 uiCircleType = nsMath::Clamp<nsUInt8>(type, 0, 8);

  const nsInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const nsInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (nsInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (nsInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      if (OverlapCircle[y][x] <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough) == nsCallbackResult::Stop)
          return nsRasterizationResult::Aborted;
      }
    }
  }

  return nsRasterizationResult::Finished;
}

nsRasterizationResult::Enum ns2DGridUtils::RasterizeBlobWithDistance(
  nsInt32 iPosX, nsInt32 iPosY, nsBlobType type, NS_RASTERIZED_BLOB_CALLBACK callback, void* pPassThrough /*= nullptr*/)
{
  const nsUInt8 uiCircleType = nsMath::Clamp<nsUInt8>(type, 0, 8);

  const nsInt32 iAreaMin = CircleAreaMin[uiCircleType];
  const nsInt32 iAreaMax = CircleAreaMax[uiCircleType];

  iPosX -= CircleCenter;
  iPosY -= CircleCenter;

  for (nsInt32 y = iAreaMin; y <= iAreaMax; ++y)
  {
    for (nsInt32 x = iAreaMin; x <= iAreaMax; ++x)
    {
      const nsUInt8 uiDistance = OverlapCircle[y][x];

      if (uiDistance <= uiCircleType)
      {
        if (callback(iPosX + x, iPosY + y, pPassThrough, uiDistance) == nsCallbackResult::Stop)
          return nsRasterizationResult::Aborted;
      }
    }
  }

  return nsRasterizationResult::Finished;
}

nsRasterizationResult::Enum ns2DGridUtils::RasterizeCircle(
  nsInt32 iPosX, nsInt32 iPosY, float fRadius, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */)
{
  const nsVec2 vCenter((float)iPosX, (float)iPosY);

  const nsInt32 iRadius = (nsInt32)fRadius;
  const float fRadiusSqr = nsMath::Square(fRadius);

  for (nsInt32 y = iPosY - iRadius; y <= iPosY + iRadius; ++y)
  {
    for (nsInt32 x = iPosX - iRadius; x <= iPosX + iRadius; ++x)
    {
      const nsVec2 v((float)x, (float)y);

      if ((v - vCenter).GetLengthSquared() > fRadiusSqr)
        continue;

      if (callback(x, y, pPassThrough) == nsCallbackResult::Stop)
        return nsRasterizationResult::Aborted;
    }
  }

  return nsRasterizationResult::Finished;
}


struct VisibilityLine
{
  nsDynamicArray<nsUInt8>* m_pVisible;
  nsUInt32 m_uiSize;
  nsUInt32 m_uiRadius;
  nsInt32 m_iCenterX;
  nsInt32 m_iCenterY;
  ns2DGridUtils::NS_RASTERIZED_POINT_CALLBACK m_VisCallback;
  void* m_pUserPassThrough;
  nsUInt32 m_uiWidth;
  nsUInt32 m_uiHeight;
  nsVec2 m_vDirection;
  nsAngle m_ConeAngle;
};

struct CellFlags
{
  enum Enum
  {
    NotVisited = 0,
    Visited = NS_BIT(0),
    Visible = Visited | NS_BIT(1),
    Invisible = Visited,
  };
};

static nsCallbackResult::Enum MarkPointsOnLineVisible(nsInt32 x, nsInt32 y, void* pPassThrough)
{
  VisibilityLine* VisLine = (VisibilityLine*)pPassThrough;

  // if the reported point is outside the playing field, don't continue
  if (x < 0 || y < 0 || x >= (nsInt32)VisLine->m_uiWidth || y >= (nsInt32)VisLine->m_uiHeight)
    return nsCallbackResult::Stop;

  // compute the point position inside our virtual grid (where the start position is at the center)
  const nsUInt32 VisX = x - VisLine->m_iCenterX + VisLine->m_uiRadius;
  const nsUInt32 VisY = y - VisLine->m_iCenterY + VisLine->m_uiRadius;

  // if we are outside our virtual grid, stop
  if (VisX >= (nsInt32)VisLine->m_uiSize || VisY >= (nsInt32)VisLine->m_uiSize)
    return nsCallbackResult::Stop;

  // We actually only need two bits for each cell (visited + visible)
  // so we pack the information for four cells into one byte
  const nsUInt32 uiCellIndex = VisY * VisLine->m_uiSize + VisX;
  const nsUInt32 uiBitfieldByte = uiCellIndex >> 2;                   // division by four
  const nsUInt32 uiBitfieldBiteOff = uiBitfieldByte << 2;             // modulo to determine where in the byte this cell is stored
  const nsUInt32 uiMaskShift = (uiCellIndex - uiBitfieldBiteOff) * 2; // times two because we use two bits

  nsUInt8& CellFlagsRef = (*VisLine->m_pVisible)[uiBitfieldByte];     // for writing into the byte later
  const nsUInt8 ThisCellsFlags = (CellFlagsRef >> uiMaskShift) & 3U;  // the decoded flags value for reading (3U == lower two bits)

  // if this point on the line was already visited and determined to be invisible, don't continue
  if (ThisCellsFlags == CellFlags::Invisible)
    return nsCallbackResult::Stop;

  // this point has been visited already and the point was determined to be visible, so just continue
  if (ThisCellsFlags == CellFlags::Visible)
    return nsCallbackResult::Continue;

  // apparently this cell has not been visited yet, so ask the user callback what to do
  if (VisLine->m_VisCallback(x, y, VisLine->m_pUserPassThrough) == nsCallbackResult::Continue)
  {
    // the callback reported this cell as visible, so flag it and continue
    CellFlagsRef |= ((nsUInt8)CellFlags::Visible) << uiMaskShift;
    return nsCallbackResult::Continue;
  }

  // the callback reported this flag as invisible, flag it and stop the line
  CellFlagsRef |= ((nsUInt8)CellFlags::Invisible) << uiMaskShift;
  return nsCallbackResult::Stop;
}

static nsCallbackResult::Enum MarkPointsInCircleVisible(nsInt32 x, nsInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  ns2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return nsCallbackResult::Continue;
}

void ns2DGridUtils::ComputeVisibleArea(nsInt32 iPosX, nsInt32 iPosY, nsUInt16 uiRadius, nsUInt32 uiWidth, nsUInt32 uiHeight,
  NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */, nsDynamicArray<nsUInt8>* pTempArray /* = nullptr */)
{
  const nsUInt32 uiSize = uiRadius * 2 + 1;

  nsDynamicArray<nsUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(nsMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte

  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;

  // from the center, trace lines to all points on the circle around it
  // each line determines for each cell whether it is visible
  // once an invisible cell is encountered, a line will stop further tracing
  // no cell is ever reported twice to the user callback
  ns2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInCircleVisible, &ld);
}

static nsCallbackResult::Enum MarkPointsInConeVisible(nsInt32 x, nsInt32 y, void* pPassThrough)
{
  VisibilityLine* ld = (VisibilityLine*)pPassThrough;

  const nsVec2 vPos((float)x, (float)y);
  const nsVec2 vDirToPos = (vPos - nsVec2((float)ld->m_iCenterX, (float)ld->m_iCenterY)).GetNormalized();

  const nsAngle angle = nsMath::ACos(vDirToPos.Dot(ld->m_vDirection));

  if (angle.GetRadian() < ld->m_ConeAngle.GetRadian())
    ns2DGridUtils::ComputePointsOnLineConservative(ld->m_iCenterX, ld->m_iCenterY, x, y, MarkPointsOnLineVisible, pPassThrough, false);

  return nsCallbackResult::Continue;
}

void ns2DGridUtils::ComputeVisibleAreaInCone(nsInt32 iPosX, nsInt32 iPosY, nsUInt16 uiRadius, const nsVec2& vDirection, nsAngle coneAngle,
  nsUInt32 uiWidth, nsUInt32 uiHeight, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough /* = nullptr */,
  nsDynamicArray<nsUInt8>* pTempArray /* = nullptr */)
{
  const nsUInt32 uiSize = uiRadius * 2 + 1;

  nsDynamicArray<nsUInt8> VisiblityFlags;

  // if we don't get a temp array, use our own array, with blackjack etc.
  if (pTempArray == nullptr)
    pTempArray = &VisiblityFlags;

  pTempArray->Clear();
  pTempArray->SetCount(nsMath::Square(uiSize) / 4); // we store only two bits per cell, so we can pack four values into each byte


  VisibilityLine ld;
  ld.m_uiSize = uiSize;
  ld.m_uiRadius = uiRadius;
  ld.m_pVisible = pTempArray;
  ld.m_iCenterX = iPosX;
  ld.m_iCenterY = iPosY;
  ld.m_VisCallback = callback;
  ld.m_pUserPassThrough = pPassThrough;
  ld.m_uiWidth = uiWidth;
  ld.m_uiHeight = uiHeight;
  ld.m_vDirection = vDirection;
  ld.m_ConeAngle = coneAngle;

  ns2DGridUtils::ComputePointsOnCircle(iPosX, iPosY, uiRadius, MarkPointsInConeVisible, &ld);
}
