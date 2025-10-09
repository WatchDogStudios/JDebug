#include <Utilities/UtilitiesPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Utilities/DGML/DGMLCreator.h>

void nsDGMLGraphCreator::FillGraphFromWorld(nsWorld* pWorld, nsDGMLGraph& ref_graph)
{
  if (!pWorld)
  {
    nsLog::Warning("nsDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(nsDGMLGraph& ref_graph)
      : m_Graph(ref_graph)
    {
      nsDGMLGraph::NodeDesc nd;
      nd.m_Color = nsColor::DarkRed;
      nd.m_Shape = nsDGMLGraph::NodeShape::Button;
      m_WorldNodeId = ref_graph.AddNode("World", &nd);
    }

    nsVisitorExecution::Enum Visit(nsGameObject* pObject)
    {
      nsStringBuilder name;
      name.SetFormat("GameObject: \"{0}\"", pObject->GetName().IsEmpty() ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      nsDGMLGraph::NodeDesc gameobjectND;
      gameobjectND.m_Color = nsColor::CornflowerBlue;
      gameobjectND.m_Shape = nsDGMLGraph::NodeShape::Rectangle;
      auto gameObjectNodeId = m_Graph.AddNode(name.GetData(), &gameobjectND);

      m_VisitedObjects.Insert(pObject, gameObjectNodeId);

      // Add connection to parent if existent
      if (const nsGameObject* parent = pObject->GetParent())
      {
        auto it = m_VisitedObjects.Find(parent);

        if (it.IsValid())
        {
          m_Graph.AddConnection(gameObjectNodeId, it.Value());
        }
      }
      else
      {
        // No parent -> connect to world
        m_Graph.AddConnection(gameObjectNodeId, m_WorldNodeId);
      }

      // Add components
      for (auto component : pObject->GetComponents())
      {
        auto sComponentName = component->GetDynamicRTTI()->GetTypeName();

        nsDGMLGraph::NodeDesc componentND;
        componentND.m_Color = nsColor::LimeGreen;
        componentND.m_Shape = nsDGMLGraph::NodeShape::RoundedRectangle;
        auto componentNodeId = m_Graph.AddNode(sComponentName, &componentND);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return nsVisitorExecution::Continue;
    }

    nsDGMLGraph& m_Graph;

    nsDGMLGraph::NodeId m_WorldNodeId;
    nsMap<const nsGameObject*, nsDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(ref_graph);
  pWorld->Traverse(nsWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), nsWorld::BreadthFirst);
}
