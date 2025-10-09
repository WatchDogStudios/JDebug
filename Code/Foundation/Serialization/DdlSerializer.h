#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class nsOpenDdlReaderElement;

/// \brief Represents a named block of serialized data within a DDL document.
///
/// DDL documents can contain multiple named blocks (Header, Objects, Types) each containing
/// an object graph. This structure holds one such block with its name and deserialized content.
struct NS_FOUNDATION_DLL nsSerializedBlock
{
  nsString m_Name;                            ///< Name of the block (e.g., "Header", "Objects", "Types")
  nsUniquePtr<nsAbstractObjectGraph> m_Graph; ///< Deserialized object graph for this block
};

/// \brief Low-level DDL serializer for nsAbstractObjectGraph instances.
///
/// This class provides efficient DDL (Data Definition Language) serialization of abstract object graphs.
/// DDL is a human-readable text format that supports comments, structured data, and type information.
class NS_FOUNDATION_DLL nsAbstractGraphDdlSerializer
{
public:
  /// \brief Writes an object graph to a DDL stream with optional type information.
  ///
  /// \param pGraph The main object graph to serialize
  /// \param pTypesGraph Optional type information for versioning support
  /// \param bCompactMmode If true, minimizes whitespace for smaller files
  /// \param typeMode Controls verbosity of type names in output
  static void Write(nsStreamWriter& inout_stream, const nsAbstractObjectGraph* pGraph, const nsAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, nsOpenDdlWriter::TypeStringMode typeMode = nsOpenDdlWriter::TypeStringMode::Shortest);

  /// \brief Reads an object graph from a DDL stream with optional patching.
  ///
  /// \param pGraph Output graph to populate with deserialized data
  /// \param pTypesGraph Optional type information output for version tracking
  /// \param bApplyPatches If true, applies version patches during deserialization
  static nsResult Read(nsStreamReader& inout_stream, nsAbstractObjectGraph* pGraph, nsAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void Write(nsOpenDdlWriter& inout_stream, const nsAbstractObjectGraph* pGraph, const nsAbstractObjectGraph* pTypesGraph = nullptr);
  static nsResult Read(const nsOpenDdlReaderElement* pRootElement, nsAbstractObjectGraph* pGraph, nsAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  /// \brief Writes a complete document with separate header, objects, and types sections.
  ///
  /// This creates a structured DDL document with named blocks for different types of data.
  /// Commonly used for asset files that need metadata (header), main content (objects),
  /// and type information (types) in separate, clearly organized sections.
  static void WriteDocument(nsStreamWriter& inout_stream, const nsAbstractObjectGraph* pHeader, const nsAbstractObjectGraph* pGraph, const nsAbstractObjectGraph* pTypes, bool bCompactMode = true, nsOpenDdlWriter::TypeStringMode typeMode = nsOpenDdlWriter::TypeStringMode::Shortest);

  /// \brief Reads a complete document and separates header, objects, and types into distinct graphs.
  static nsResult ReadDocument(nsStreamReader& inout_stream, nsUniquePtr<nsAbstractObjectGraph>& ref_pHeader, nsUniquePtr<nsAbstractObjectGraph>& ref_pGraph, nsUniquePtr<nsAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  /// \brief Reads only the header section from a document without processing the full content.
  ///
  /// This is useful for quickly extracting metadata or file information without the overhead
  /// of deserializing the entire document. Commonly used for asset browsers and file inspection.
  static nsResult ReadHeader(nsStreamReader& inout_stream, nsAbstractObjectGraph* pGraph);

private:
  static nsResult ReadBlocks(nsStreamReader& stream, nsHybridArray<nsSerializedBlock, 3>& blocks);
};
