#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/ObjectSelection.h>

nsObjectSelection::nsObjectSelection()
{
  m_pWorld = nullptr;
}

void nsObjectSelection::SetWorld(nsWorld* pWorld)
{
  NS_ASSERT_DEV((m_pWorld == pWorld) || m_Objects.IsEmpty(), "The selection has to be empty to change the world.");

  m_pWorld = pWorld;
}

void nsObjectSelection::RemoveDeadObjects()
{
  NS_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  for (nsUInt32 i = m_Objects.GetCount(); i > 0; --i)
  {
    nsGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAtAndCopy(i - 1); // keep the order
    }
  }
}

void nsObjectSelection::AddObject(nsGameObjectHandle hObject, bool bDontAddTwice)
{
  NS_IGNORE_UNUSED(bDontAddTwice);
  NS_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  // only insert valid objects
  nsGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  if (m_Objects.IndexOf(hObject) != nsInvalidIndex)
    return;

  m_Objects.PushBack(hObject);
}

bool nsObjectSelection::RemoveObject(nsGameObjectHandle hObject)
{
  return m_Objects.RemoveAndCopy(hObject);
}

void nsObjectSelection::ToggleSelection(nsGameObjectHandle hObject)
{
  for (nsUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAtAndCopy(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}
