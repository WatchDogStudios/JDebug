#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class nsDocument;
struct nsDocumentObjectStructureEvent;


/// \brief Event describing changes to the selection in the selection manager.
struct nsSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
    ChangedRuntimeOverrideSelection, ///< Broadcast by SetRuntimeOverrideSelection().
  };

  Type m_Type;
  const nsDocument* m_pDocument;
  const nsDocumentObject* m_pObject;
};

struct nsSelectionEntry
{
  const nsDocumentObject* m_pObject;
  nsUInt32 m_uiSelectionOrder = 0; // the index at which this item was in the selection
};

/// \brief Selection Manager stores a set of selected document objects.
class NS_TOOLSFOUNDATION_DLL nsSelectionManager
{
public:
  /// \brief Event that is broadcast when the selection changes.
  nsCopyOnBroadcastEvent<const nsSelectionManagerEvent&> m_Events;

  /// \brief Storage for the selection so it can be swapped when using multiple sub documents.
  class Storage : public nsRefCounted
  {
  public:
    nsDeque<const nsDocumentObject*> m_SelectionList;
    nsSet<nsUuid> m_SelectionSet;
    const nsDocumentObjectManager* m_pObjectManager = nullptr;
    nsCopyOnBroadcastEvent<const nsSelectionManagerEvent&> m_Events;
  };

public:
  nsSelectionManager(const nsDocumentObjectManager* pObjectManager);
  ~nsSelectionManager();

  void Clear();
  void AddObject(const nsDocumentObject* pObject);
  void RemoveObject(const nsDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const nsDocumentObject* pSingleObject);
  void SetSelection(const nsDeque<const nsDocumentObject*>& selection);
  void ToggleObject(const nsDocumentObject* pObject);

  /// \brief Sets a separate selection (temporarily), which is sent to the engine but not propagated to the editor.
  ///
  /// This is used for cases where temporarily the engine should use a different selection than the editor.
  /// Currently this is used during drag-and-drop, to already show the dragged object as selected and especially to exclude it from picking,
  /// but not yet show the new object as selected in the property grids, such that users can interact with the previously selected object.
  ///
  /// To clear a runtime override selection, simply set an empty selection.
  void SetRuntimeOverrideSelection(const nsDeque<const nsDocumentObject*>& selection);

  /// \brief Returns the current runtime override selection.
  ///
  /// Valid, if the selection is non-empty.
  /// See SetRuntimeOverrideSelection() for details.
  const nsDeque<const nsDocumentObject*>& GetRuntimeOverrideSelection() const { return m_RuntimeOverrideSelection; }

  /// \brief Returns the last selected object in the selection or null if empty.
  const nsDocumentObject* GetCurrentObject() const;

  /// \brief Returns the selection in the same order the objects were added to the list.
  const nsDeque<const nsDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }



  /// \brief Returns the subset of selected items which have no parent selected.
  ///
  /// I.e. if an object is selected and one of its ancestors is selected, it is culled from the list.
  /// Items are returned in the order of appearance in an expanded scene tree.
  /// Their order in the selection is returned through nsSelectionEntry.
  void GetTopLevelSelection(nsDynamicArray<nsSelectionEntry>& out_entries) const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  void GetTopLevelSelectionOfType(const nsRTTI* pBase, nsDynamicArray<nsSelectionEntry>& out_entries) const;

  bool IsSelected(const nsDocumentObject* pObject) const;
  bool IsParentSelected(const nsDocumentObject* pObject) const;

  const nsDocument* GetDocument() const;

  nsSharedPtr<nsSelectionManager::Storage> SwapStorage(nsSharedPtr<nsSelectionManager::Storage> pNewStorage);
  nsSharedPtr<nsSelectionManager::Storage> GetStorage() { return m_pSelectionStorage; }

private:
  void TreeEventHandler(const nsDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const nsDocumentObject* pObject);

  friend class nsDocument;

  nsSharedPtr<nsSelectionManager::Storage> m_pSelectionStorage;
  nsDeque<const nsDocumentObject*> m_RuntimeOverrideSelection;

  nsCopyOnBroadcastEvent<const nsDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  nsCopyOnBroadcastEvent<const nsSelectionManagerEvent&>::Unsubscriber m_EventsUnsubscriber;
};
