#pragma once

#include <Foundation/Serialization/RttiConverter.h>

/// \brief Specialized context for tracking and applying native object changes to abstract object graphs.
///
/// This context enables a sophisticated bidirectional synchronization workflow between native
/// C++ objects and their serialized representations in nsAbstractObjectGraph form. It ensures
/// that modifications made to native objects can be properly tracked and applied back to the
/// abstract representation while maintaining object identity through consistent GUID generation.
///
/// The key capability is generating GUIDs for native objects that exactly match the GUIDs
/// used in the original abstract object graph. This allows the system to correlate changes
/// made to native objects with their counterparts in the serialized form.
///
/// Typical workflow:
/// 1. Deserialize an abstract object graph to native objects
/// 2. Create this context to track GUID relationships
/// 3. Modify the native objects through normal C++ operations
/// 4. Use the context to detect and apply changes back to the abstract graph
/// 5. Serialize the updated abstract graph for persistence
///
/// This is particularly useful for:
/// - Editor scenarios where objects are modified through UI and need to be saved
/// - Undo/redo systems that operate on abstract object graphs
/// - Network synchronization where changes need to be transmitted efficiently
/// - Asset pipeline where native modifications need to be persisted
///
/// \sa nsAbstractObjectGraph::ModifyNodeViaNativeCounterpart
class NS_FOUNDATION_DLL nsApplyNativePropertyChangesContext : public nsRttiConverterContext
{
public:
  nsApplyNativePropertyChangesContext(nsRttiConverterContext& ref_source, const nsAbstractObjectGraph& originalGraph);

  virtual nsUuid GenerateObjectGuid(const nsUuid& parentGuid, const nsAbstractProperty* pProp, nsVariant index, void* pObject) const override;

private:
  nsRttiConverterContext& m_NativeContext;
  const nsAbstractObjectGraph& m_OriginalGraph;
};
