#pragma once

#include <Foundation/Math/Math.h>
#include <Utilities/PathFinding/PathState.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Implements a directed breadth-first search through a graph (A*).
///
/// You can search for a path to a specific location using FindPath() or to the closest node that fulfills some arbitrary criteria
/// using FindClosest().
///
/// PathStateType must be derived from nsPathState and can be used for keeping track of certain state along a path and to modify
/// the path search dynamically.
template <typename PathStateType>
class nsPathSearch
{
public:
  /// \brief Used by FindClosest() to query whether the currently visited node fulfills the termination criteria.
  using IsSearchedObjectCallback = bool (*)(nsInt64 iStartNodeIndex, const PathStateType& StartState);

  /// \brief FindPath() and FindClosest() return an array of these objects as the path result.
  struct PathResultData
  {
    NS_DECLARE_POD_TYPE();

    /// \brief The index of the node that was visited.
    nsInt64 m_iNodeIndex;

    /// \brief Pointer to the path state that was active at that step along the path.
    const PathStateType* m_pPathState;
  };

  /// \brief Sets the nsPathStateGenerator that should be used by this nsPathSearch object.
  void SetPathStateGenerator(nsPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when the graph node \a iTargetNodeIndex was reached.
  ///
  /// Returns NS_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  nsResult FindPath(nsInt64 iStartNodeIndex, const PathStateType& StartState, nsInt64 iTargetNodeIndex, nsDeque<PathResultData>& out_Path,
    float fMaxPathCost = nsMath::Infinity<float>());

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when a graph node is reached for which \a Callback return true.
  ///
  /// Returns NS_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  nsResult FindClosest(nsInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, nsDeque<PathResultData>& out_Path,
    float fMaxPathCost = nsMath::Infinity<float>());

  /// \brief Needs to be called by the used nsPathStateGenerator to add nodes to evaluate.
  void AddPathNode(nsInt64 iNodeIndex, const PathStateType& NewState);

private:
  void ClearPathStates();
  nsInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void FillOutPathResult(nsInt64 iEndNodeIndex, nsDeque<PathResultData>& out_Path);

  nsPathStateGenerator<PathStateType>* m_pStateGenerator;

  nsHashTable<nsInt64, PathStateType> m_PathStates;

  nsDeque<nsInt64> m_StateQueue;

  nsInt64 m_iCurNodeIndex;
  PathStateType m_CurState;
};



#include <Utilities/PathFinding/Implementation/GraphSearch_inl.h>
