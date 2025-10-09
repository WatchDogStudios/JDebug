#pragma once

template <class CellData>
void nsGridNavmesh::CreateFromGrid(
  const nsGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThrough, CellBlocked isCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(grid.GetGridSizeX(), grid.GetGridSizeY());

  UpdateRegion(nsRectU32(grid.GetGridSizeX(), grid.GetGridSizeY()), isSameCellType, pPassThrough, isCellBlocked, pPassThrough2);

  CreateGraphEdges();
}
