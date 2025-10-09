#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
struct nsCallbackResult
{
  enum Enum
  {
    Stop,     ///< The calling function should stop expanding in this direction (might mean it should abort entirely)
    Continue, ///< The calling function should continue further.
  };
};

/// \brief Enum values for the result of some rasterization functions.
struct nsRasterizationResult
{
  enum Enum
  {
    Aborted,  ///< The function was aborted before it reached the end.
    Finished, ///< The function rasterized all possible points.
  };
};

namespace ns2DGridUtils
{
  /// \brief The callback declaration for the function that needs to be passed to the various rasterization functions.
  using NS_RASTERIZED_POINT_CALLBACK = nsCallbackResult::Enum (*)(nsInt32, nsInt32, void*);

  /// \brief The callback declaration for the function that needs to be passed to RasterizeBlobWithDistance().
  using NS_RASTERIZED_BLOB_CALLBACK = nsCallbackResult::Enum (*)(nsInt32, nsInt32, void*, nsUInt8);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// The function implements Bresenham's algorithm for line rasterization. The first point to be reported through the
  /// callback is always the start position, the last point is always the end position.
  /// pPassThrough is passed through to the user callback for custom data.
  ///
  /// The function returns nsRasterizationResult::Aborted if the callback returned nsCallbackResult::Stop at any time
  /// and the line will not be computed further in that case.
  /// It returns nsRasterizationResult::Finished if the entire line was rasterized.
  ///
  /// This function does not do any dynamic memory allocations internally.
  NS_UTILITIES_DLL nsRasterizationResult::Enum ComputePointsOnLine(
    nsInt32 iStartX, nsInt32 iStartY, nsInt32 iEndX, nsInt32 iEndY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Computes all the points on a 2D line and calls a function to report every point.
  ///
  /// Contrary to ComputePointsOnLine() this function does not do diagonal steps but inserts horizontal or vertical steps, such that
  /// reported cells are always connected by an edge.
  /// However, since there are always two possibilities to go from one cell to a diagonal cell, this function tries both and as long
  /// as one of them reports nsCallbackResult::Continue, it will continue. Only if both cells are blocked will the algorithm abort.
  ///
  /// If bVisitBothNeighbors is false, the line will continue with the diagonal cell if the first tried neighbor cell is free.
  /// However, if bVisitBothNeighbors is true, the second alternative cell is also reported to the callback, even though its return value
  /// has no effect on whether the line continues or aborts.
  NS_UTILITIES_DLL nsRasterizationResult::Enum ComputePointsOnLineConservative(nsInt32 iStartX, nsInt32 iStartY, nsInt32 iEndX, nsInt32 iEndY,
    NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, bool bVisitBothNeighbors = false);

  /// \brief Computes all the points on a 2D circle and calls a function to report every point.
  ///
  /// The points are reported in a rather chaotic order (ie. when one draws a line from point to point, it does not yield a circle shape).
  /// The callback may abort the operation by returning nsCallbackResult::Stop.
  ///
  /// This function does not do any dynamic memory allocations internally.
  NS_UTILITIES_DLL nsRasterizationResult::Enum ComputePointsOnCircle(
    nsInt32 iStartX, nsInt32 iStartY, nsUInt32 uiRadius, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Starts at the given point and then fills all surrounding cells until a border is detected.
  ///
  /// The callback should return nsCallbackResult::Continue for each cell that has not been visited so far and for which all four direct
  /// neighbors should be visited. If the flood-fill algorithm leaves the valid area, the callback must return nsCallbackResult::Stop to
  /// signal a border. Thus the callback must be able to handle point positions outside the valid range and it also needs to be able to
  /// detect which cells have been visited before, as the FloodFill function will not keep that state internally.
  ///
  /// The function returns the number of cells that were visited and returned nsCallbackResult::Continue (ie. which were not classified as
  /// border cells).
  ///
  /// Note that the FloodFill function requires an internal queue to store which cells still need to be visited, as such it will do
  /// dynamic memory allocations. You can pass in a queue that will be used as the temp buffer, thus you can reuse the same container for
  /// several operations, which will reduce the amount of memory allocations that need to be done.
  NS_UTILITIES_DLL nsUInt32 FloodFill(
    nsInt32 iStartX, nsInt32 iStartY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, nsDeque<nsVec2I32>* pTempArray = nullptr);

  /// \brief Same as FloodFill() but also visits the diagonal neighbors, ie. all eight neighboring cells.
  NS_UTILITIES_DLL nsUInt32 FloodFillDiag(
    nsInt32 iStartX, nsInt32 iStartY, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, nsDeque<nsVec2I32>* pTempArray = nullptr);

  /// \brief Describes the different circle types that can be rasterized
  enum nsBlobType : nsUInt8
  {
    Point1x1,    ///< The circle has just one point at the center
    Cross3x3,    ///< The circle has 5 points, one at the center, 1 at each edge of that
    Block3x3,    ///< The 'circle' is just a 3x3 rectangle (9 points)
    Circle5x5,   ///< The circle is a rectangle with each of the 4 corner points missing (21 points)
    Circle7x7,   ///< The circle is a actually starts looking like a circle (37 points)
    Circle9x9,   ///< Circle with 57 points
    Circle11x11, ///< Circle with 97 points
    Circle13x13, ///< Circle with 129 points
    Circle15x15, ///< Circle with 177 points
  };

  /// \brief Rasterizes a circle of limited dimensions and calls the given callback for each point.
  ///
  /// See nsCircleType for the available circle types. Those circles are handcrafted to have good looking shapes at low resolutions.
  /// This type of circle is not meant for actually rendering circles, but for doing area operations and overlapping checks for game
  /// units, visibility determination etc. Basically everything that is usually small, but where a simple point might not suffice.
  /// For example most units in a strategy game might only occupy a single cell, but some units might be larger and thus need to occupy
  /// the surrounding cells as well. Using RasterizeBlob() you can compute the units footprint easily.
  ///
  /// RasterizeBlob() will stop immediately and return nsRasterizationResult::Aborted when the callback function returns
  /// nsCallbackResult::Stop.
  NS_UTILITIES_DLL nsRasterizationResult::Enum RasterizeBlob(
    nsInt32 iPosX, nsInt32 iPosY, nsBlobType type, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Same as RasterizeBlob(), but the distance from the center is passed through to the callback, which can use this information to
  /// adjust what it is doing.
  NS_UTILITIES_DLL nsRasterizationResult::Enum RasterizeBlobWithDistance(
    nsInt32 iPosX, nsInt32 iPosY, nsBlobType type, NS_RASTERIZED_BLOB_CALLBACK callback, void* pPassThrough = nullptr);

  /// \brief Rasterizes a circle of any size (unlike RasterizeBlob()), though finding the right radius values for nice looking small circles
  /// can be more difficult.
  ///
  /// This function rasterizes a full circle. The radius is a float value, ie. you can use fractional values to shave off cells at the
  /// borders bit by bit.
  ///
  /// RasterizeCircle() will stop immediately and return nsRasterizationResult::Aborted when the callback function returns
  /// nsCallbackResult::Stop.
  NS_UTILITIES_DLL nsRasterizationResult::Enum RasterizeCircle(
    nsInt32 iPosX, nsInt32 iPosY, float fRadius, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr);


  /// \brief Computes which points are visible from the start position by tracing lines radially outwards.
  ///
  /// The center start position is at iPosX, iPosY and uiRadius defines the maximum distance that an object can see.
  /// uiWidth and uiHeight define the maximum coordinates at which the end of the grid is reached (and thus the line tracing can early out
  /// if it reaches those). For the minimum coordinate (0, 0) is assumed.
  ///
  /// The callback function must return nsCallbackResult::Continue for cells that are not blocking and nsCallbackResult::Stop for cells that
  /// block visibility.
  ///
  /// The algorithm requires internal state and thus needs to do dynamic memory allocations. If you want to reduce the number of
  /// allocations, you can pass in your own array, that can be reused for many queries.
  NS_UTILITIES_DLL void ComputeVisibleArea(nsInt32 iPosX, nsInt32 iPosY, nsUInt16 uiRadius, nsUInt32 uiWidth, nsUInt32 uiHeight,
    NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr, nsDynamicArray<nsUInt8>* pTempArray = nullptr);

  /// \brief Computes which points are visible from the start position by tracing lines radially outwards. Limits the computation to a cone.
  ///
  /// This function works exactly like ComputeVisibleArea() but limits the computation to a cone that is defined by vDirection and
  /// ConeAngle.
  NS_UTILITIES_DLL void ComputeVisibleAreaInCone(nsInt32 iPosX, nsInt32 iPosY, nsUInt16 uiRadius, const nsVec2& vDirection, nsAngle coneAngle,
    nsUInt32 uiWidth, nsUInt32 uiHeight, NS_RASTERIZED_POINT_CALLBACK callback, void* pPassThrough = nullptr,
    nsDynamicArray<nsUInt8>* pTempArray = nullptr);
} // namespace ns2DGridUtils
