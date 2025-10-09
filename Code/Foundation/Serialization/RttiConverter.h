#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class nsAbstractObjectGraph;
class nsAbstractObjectNode;

/// \brief Simple wrapper that pairs a runtime type with an object instance pointer.
///
/// This structure is used throughout the RTTI converter system to maintain type safety
/// when working with void pointers. It ensures that object pointers are always associated
/// with their correct runtime type information.
struct NS_FOUNDATION_DLL nsRttiConverterObject
{
  nsRttiConverterObject()
    : m_pType(nullptr)
    , m_pObject(nullptr)
  {
  }
  nsRttiConverterObject(const nsRTTI* pType, void* pObject)
    : m_pType(pType)
    , m_pObject(pObject)
  {
  }

  NS_DECLARE_POD_TYPE();

  const nsRTTI* m_pType; ///< Runtime type information for the object
  void* m_pObject;       ///< Pointer to the actual object instance
};


/// \brief Context object that manages object lifetime and relationships during RTTI-based conversion.
///
/// This class provides the infrastructure for converting between native objects and abstract
/// object graphs. It handles object creation, deletion, GUID management, and type resolution
/// during both serialization and deserialization processes.
///
/// Key responsibilities:
/// - Object lifecycle management (creation, registration, deletion)
/// - GUID generation and object-to-GUID mapping
/// - Type resolution and unknown type handling
/// - Object queuing for deferred processing
/// - Cross-reference resolution during deserialization
///
/// The context can be customized by overriding virtual methods to implement:
/// - Custom GUID generation strategies
/// - Alternative object creation patterns
/// - Specialized type resolution logic
/// - Custom error handling for unknown types
class NS_FOUNDATION_DLL nsRttiConverterContext
{
public:
  /// \brief Clears all cached objects and resets the context state.
  virtual void Clear();

  /// \brief Generates a guid for a new object. Default implementation generates stable guids derived from
  /// parentGuid + property name + index and ignores the address of pObject.
  virtual nsUuid GenerateObjectGuid(const nsUuid& parentGuid, const nsAbstractProperty* pProp, nsVariant index, void* pObject) const;

  virtual nsInternal::NewInstance<void> CreateObject(const nsUuid& guid, const nsRTTI* pRtti);
  virtual void DeleteObject(const nsUuid& guid);

  virtual void RegisterObject(const nsUuid& guid, const nsRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const nsUuid& guid);

  virtual nsRttiConverterObject GetObjectByGUID(const nsUuid& guid) const;
  virtual nsUuid GetObjectGUID(const nsRTTI* pRtti, const void* pObject) const;

  virtual const nsRTTI* FindTypeByName(nsStringView sName) const;

  template <typename T>
  void GetObjectsByType(nsDynamicArray<T*>& out_objects, nsDynamicArray<nsUuid>* out_pUuids = nullptr)
  {
    for (auto it : m_GuidToObject)
    {
      if (it.Value().m_pType->IsDerivedFrom(nsGetStaticRTTI<T>()))
      {
        out_objects.PushBack(static_cast<T*>(it.Value().m_pObject));
        if (out_pUuids)
        {
          out_pUuids->PushBack(it.Key());
        }
      }
    }
  }

  virtual nsUuid EnqueObject(const nsUuid& guid, const nsRTTI* pRtti, void* pObject);
  virtual nsRttiConverterObject DequeueObject();

  virtual void OnUnknownTypeError(nsStringView sTypeName);

protected:
  nsHashTable<nsUuid, nsRttiConverterObject> m_GuidToObject;
  mutable nsHashTable<const void*, nsUuid> m_ObjectToGuid;
  nsSet<nsUuid> m_QueuedObjects;
};


/// \brief Converts native objects to abstract object graph representation using reflection.
///
/// This class traverses object hierarchies using RTTI and converts them into abstract
/// object graphs that can be serialized to various formats. It handles object references,
/// inheritance hierarchies, and complex property types automatically.
class NS_FOUNDATION_DLL nsRttiConverterWriter
{
public:
  /// \brief Filter function type for controlling which properties are serialized.
  ///
  /// Return true to include the property, false to skip it. Allows fine-grained control
  /// over what gets serialized based on object state, property attributes, or other criteria.
  using FilterFunction = nsDelegate<bool(const void* pObject, const nsAbstractProperty* pProp)>;

  /// \brief Constructs a writer with boolean flags for common filtering options.
  ///
  /// \param bSerializeReadOnly If true, includes read-only properties in the output
  /// \param bSerializeOwnerPtrs If true, serializes objects pointed to by owner pointers
  nsRttiConverterWriter(nsAbstractObjectGraph* pGraph, nsRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);

  /// \brief Constructs a writer with a custom filter function for maximum control.
  ///
  /// The filter function is called for each property and can implement complex logic
  /// to determine what should be serialized.
  nsRttiConverterWriter(nsAbstractObjectGraph* pGraph, nsRttiConverterContext* pContext, FilterFunction filter);

  nsAbstractObjectNode* AddObjectToGraph(nsReflectedClass* pObject, const char* szNodeName = nullptr)
  {
    return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, szNodeName);
  }
  nsAbstractObjectNode* AddObjectToGraph(const nsRTTI* pRtti, const void* pObject, const char* szNodeName = nullptr);

  void AddProperty(nsAbstractObjectNode* pNode, const nsAbstractProperty* pProp, const void* pObject);
  void AddProperties(nsAbstractObjectNode* pNode, const nsRTTI* pRtti, const void* pObject);

  nsAbstractObjectNode* AddSubObjectToGraph(const nsRTTI* pRtti, const void* pObject, const nsUuid& guid, const char* szNodeName);

private:
  nsRttiConverterContext* m_pContext = nullptr;
  nsAbstractObjectGraph* m_pGraph = nullptr;
  FilterFunction m_Filter;
};

/// \brief Converts abstract object graphs back to native objects using reflection.
///
/// This class performs the reverse operation of nsRttiConverterWriter, reconstructing
/// native object hierarchies from abstract object graphs. It handles object creation,
/// property restoration, and reference resolution automatically.
class NS_FOUNDATION_DLL nsRttiConverterReader
{
public:
  /// \brief Constructs a reader for the given object graph and context.
  nsRttiConverterReader(const nsAbstractObjectGraph* pGraph, nsRttiConverterContext* pContext);

  nsInternal::NewInstance<void> CreateObjectFromNode(const nsAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const nsAbstractObjectNode* pNode, const nsRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const nsAbstractProperty* pProperty, const nsAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const nsAbstractObjectNode* pNode, const nsRTTI* pRtti, void* pObject);

  nsRttiConverterContext* m_pContext = nullptr;
  const nsAbstractObjectGraph* m_pGraph = nullptr;
};
