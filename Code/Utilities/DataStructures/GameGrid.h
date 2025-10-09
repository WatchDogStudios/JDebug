#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief nsGameGrid is a general purpose 2D grid structure that has several convenience functions which are often required when working
/// with a grid.
template <class CellData>
class nsGameGrid
{
public:
  enum Orientation
  {
    InPlaneXY,      ///< The grid is expected to lie in the XY plane in world-space (when Y is up, this is similar to a 2D side scroller)
    InPlaneXZ,      ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
    InPlaneXminusZ, ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
  };

  nsGameGrid();

  /// \brief Clears all data and reallocates the grid with the given dimensions.
  void CreateGrid(nsUInt16 uiSizeX, nsUInt16 uiSizeY);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const nsVec3& vLowerLeftCorner, const nsVec3& vCellSize, Orientation ori = InPlaneXZ);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const nsVec3& vLowerLeftCorner, const nsVec3& vCellSize, const nsMat3& mRotation);

  /// \brief Returns the size of each cell.
  nsVec3 GetCellSize() const { return m_vLocalSpaceCellSize; }

  /// \brief Returns the coordinate of the cell at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  nsVec2I32 GetCellAtWorldPosition(const nsVec3& vWorldSpacePos) const;

  /// \brief Returns the number of cells along the X axis.
  nsUInt16 GetGridSizeX() const { return m_uiGridSizeX; }

  /// \brief Returns the number of cells along the Y axis.
  nsUInt16 GetGridSizeY() const { return m_uiGridSizeY; }

  /// \brief Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  nsBoundingBox GetWorldBoundingBox() const;

  /// \brief Returns the total number of cells.
  nsUInt32 GetNumCells() const { return m_uiGridSizeX * m_uiGridSizeY; }

  /// \brief Gives access to a cell by cell index.
  CellData& GetCell(nsUInt32 uiIndex) { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell index.
  const CellData& GetCell(nsUInt32 uiIndex) const { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell coordinates.
  CellData& GetCell(const nsVec2I32& vCoord) { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// \brief Gives access to a cell by cell coordinates.
  const CellData& GetCell(const nsVec2I32& vCoord) const { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// \brief Converts a cell index into a 2D cell coordinate.
  nsVec2I32 ConvertCellIndexToCoordinate(nsUInt32 uiIndex) const { return nsVec2I32(uiIndex % m_uiGridSizeX, uiIndex / m_uiGridSizeX); }

  /// \brief Converts a cell coordinate into a cell index.
  nsUInt32 ConvertCellCoordinateToIndex(const nsVec2I32& vCoord) const { return vCoord.y * m_uiGridSizeX + vCoord.x; }

  /// \brief Returns the lower left world space position of the cell with the given coordinates.
  nsVec3 GetCellWorldSpaceOrigin(const nsVec2I32& vCoord) const;
  nsVec3 GetCellLocalSpaceOrigin(const nsVec2I32& vCoord) const;

  /// \brief Returns the center world space position of the cell with the given coordinates.
  nsVec3 GetCellWorldSpaceCenter(const nsVec2I32& vCoord) const;
  nsVec3 GetCellLocalSpaceCenter(const nsVec2I32& vCoord) const;

  /// \brief Checks whether the given cell coordinate is inside valid ranges.
  bool IsValidCellCoordinate(const nsVec2I32& vCoord) const;

  /// \brief Casts a world space ray through the grid and determines which cell is hit (if any).
  ///
  /// \note The picked cell is determined from where the ray hits the 'ground plane', ie. the plane that goes through the world space origin.
  /// \note Returns true, if the ray would hit the ground, at all. The returned cell coordinate may not be valid (outside the valid range).
  ///       Call IsValidCellCoordinate() to check.
  bool PickCell(const nsVec3& vRayStartPos, const nsVec3& vRayDirNorm, nsVec2I32* out_pCellCoord, nsVec3* out_pIntersection = nullptr) const;

  /// \brief Returns the lower left corner position in world space of the grid
  const nsVec3& GetWorldSpaceOrigin() const { return m_vWorldSpaceOrigin; }

  /// \brief Returns the matrix used to rotate coordinates from grid space to world space
  const nsMat3& GetRotationToWorldSpace() const { return m_mRotateToWorldspace; }

  /// \brief Returns the matrix used to rotate coordinates from world space to grid space
  const nsMat3& GetRotationToGridSpace() const { return m_mRotateToGridspace; }

  /// \brief Tests where and at which cell the given world space ray intersects the grids bounding box
  bool GetRayIntersection(const nsVec3& vRayStartWorldSpace, const nsVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection,
    nsVec2I32& out_vCellCoord) const;

  /// \brief Tests whether a ray would hit the grid bounding box, if it were expanded by a constant.
  bool GetRayIntersectionExpandedBBox(const nsVec3& vRayStartWorldSpace, const nsVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
    float& out_fIntersection, const nsVec3& vExpandBBoxByThis) const;

  void ComputeWorldSpaceCorners(nsVec3* pCorners) const;

  nsResult Serialize(nsStreamWriter& ref_stream) const;
  nsResult Deserialize(nsStreamReader& ref_stream);

private:
  nsUInt16 m_uiGridSizeX;
  nsUInt16 m_uiGridSizeY;

  nsMat3 m_mRotateToWorldspace;
  nsMat3 m_mRotateToGridspace;

  nsVec3 m_vWorldSpaceOrigin;
  nsVec3 m_vLocalSpaceCellSize;
  nsVec3 m_vInverseLocalSpaceCellSize;

  nsDynamicArray<CellData> m_Cells;
};

#include <Utilities/DataStructures/Implementation/GameGrid_inl.h>
