#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class nsDocumentObjectManager;
class nsDocumentObject;
class nsRTTI;

/// \brief Provides helper functions for serializing document object types and copying properties between objects.
///
/// Also check out nsToolsReflectionUtils for related functionality.
class NS_TOOLSFOUNDATION_DLL nsToolsSerializationUtils
{
public:
  using FilterFunction = nsDelegate<bool(const nsAbstractProperty*)>;

  /// \brief Serializes the given set of types into the provided object graph.
  static void SerializeTypes(const nsSet<const nsRTTI*>& types, nsAbstractObjectGraph& ref_typesGraph);

  /// \brief Copies properties from a source document object to a target object, optionally filtering properties.
  static void CopyProperties(const nsDocumentObject* pSource, const nsDocumentObjectManager* pSourceManager, void* pTarget, const nsRTTI* pTargetType, FilterFunction propertFilter = nullptr);
};
