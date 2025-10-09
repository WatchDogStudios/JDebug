#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Rect.h>
#include <Utilities/DataStructures/GameGrid.h>

/// \brief Takes an nsGameGrid and creates an optimized navmesh structure from it, that is more efficient for path searches.
class NS_UTILITIES_DLL nsGridNavmesh
{
public:
  struct ConvexArea
  {
    NS_DECLARE_POD_TYPE();

    /// The space that is enclosed by this convex area.
    nsRectU32 m_Rect;

    /// The first AreaEdge that belongs to this ConvexArea.
    nsUInt32 m_uiFirstEdge;

    /// The number of AreaEdge's that belong to this ConvexArea.
    nsUInt32 m_uiNumEdges;
  };

  struct AreaEdge
  {
    NS_DECLARE_POD_TYPE();

    /// The 'area' of the edge. This is a one cell wide line that is always WITHIN the ConvexArea from where the edge connects to a neighbor
    /// area.
    nsRectU16 m_EdgeRect;

    /// The index of the area that can be reached over this edge. This is always a valid index.
    nsInt32 m_iNeighborArea;
  };

  /// \brief Callback that determines whether the cell with index \a uiCell1 and the cell with index \a uiCell2 represent the same type of
  /// terrain.
  using CellComparator = bool (*)(nsUInt32, nsUInt32, void*);

  /// \brief Callback that determines whether the cell with index \a uiCell is blocked entirely (for every type of unit) and therefore can
  /// be optimized away.
  using CellBlocked = bool (*)(nsUInt32, void*);

  /// \brief Creates the navmesh from the given nsGameGrid.
  template <class CellData>
  void CreateFromGrid(
    const nsGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThroughSame, CellBlocked isCellBlocked, void* pPassThroughBlocked);

  /// \brief Returns the index of the ConvexArea at the given cell coordinates. Negative, if the cell is blocked.
  nsInt32 GetAreaAt(const nsVec2I32& vCoord) const { return m_NodesGrid.GetCell(vCoord); }

  /// \brief Returns the number of convex areas that this navmesh consists of.
  nsUInt32 GetNumConvexAreas() const { return m_ConvexAreas.GetCount(); }

  /// \brief Returns the given convex area by index.
  const ConvexArea& GetConvexArea(nsInt32 iArea) const { return m_ConvexAreas[iArea]; }

  /// \brief Returns the number of edges between convex areas.
  nsUInt32 GetNumAreaEdges() const { return m_GraphEdges.GetCount(); }

  /// \brief Returns the given area edge by index.
  const AreaEdge& GetAreaEdge(nsInt32 iAreaEdge) const { return m_GraphEdges[iAreaEdge]; }

private:
  void UpdateRegion(nsRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2);

  void Optimize(nsRectU32 region, CellComparator IsSameCellType, void* pPassThrough);
  bool OptimizeBoxes(nsRectU32 region, CellComparator IsSameCellType, void* pPassThrough, nsUInt32 uiIntervalX, nsUInt32 uiIntervalY,
    nsUInt32 uiWidth, nsUInt32 uiHeight, nsUInt32 uiOffsetX = 0, nsUInt32 uiOffsetY = 0);
  bool CanCreateArea(nsRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const;

  bool CanMergeRight(nsInt32 x, nsInt32 y, CellComparator IsSameCellType, void* pPassThrough, nsRectU32& out_Result) const;
  bool CanMergeDown(nsInt32 x, nsInt32 y, CellComparator IsSameCellType, void* pPassThrough, nsRectU32& out_Result) const;
  bool MergeBestFit(nsRectU32 region, CellComparator IsSameCellType, void* pPassThrough);

  void CreateGraphEdges();
  void CreateGraphEdges(ConvexArea& Area);

  nsRectU32 GetCellBBox(nsInt32 x, nsInt32 y) const;
  void Merge(const nsRectU32& rect);
  void CreateNodes(nsRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough);

  nsGameGrid<nsInt32> m_NodesGrid;
  nsDynamicArray<ConvexArea> m_ConvexAreas;
  nsDeque<AreaEdge> m_GraphEdges;
};

#include <Utilities/PathFinding/Implementation/GridNavmesh_inl.h>
