#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>

class nsLogInterface;

/// \brief The primitive data types that OpenDDL supports
enum class nsOpenDdlPrimitiveType
{
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  // Half, // Currently not supported
  Float,
  Double,
  String,
  // Ref, // Currently not supported
  // Type // Currently not supported
  Custom
};

/// \brief Low-level streaming parser for OpenDDL documents
///
/// This abstract base class provides incremental parsing of OpenDDL (Open Data Description Language) format.
/// Unlike nsOpenDdlReader which builds a complete in-memory tree, this parser operates in streaming mode,
/// calling virtual functions as elements are encountered. This allows processing large documents with minimal
/// memory usage and enables selective parsing where only certain parts are processed.
/// Derived classes must override the virtual On* methods to handle parsed elements.
class NS_FOUNDATION_DLL nsOpenDdlParser
{
public:
  nsOpenDdlParser();
  virtual ~nsOpenDdlParser() = default;

  /// \brief Whether an error occurred during parsing that resulted in cancellation of further parsing.
  bool HadFatalParsingError() const { return m_bHadFatalParsingError; } // [tested]

protected:
  /// \brief Sets an nsLogInterface through which errors and warnings are reported.
  void SetLogInterface(nsLogInterface* pLog) { m_pLogInterface = pLog; }

  /// \brief Sets the internal cache size for batching primitive data callbacks
  ///
  /// Data is returned in larger chunks to reduce the number of function calls. The cache size determines
  /// the maximum chunk size per primitive type. Default cache size is 4 KB, allowing up to 1000 integers
  /// or 500 doubles per chunk. Increasing the cache size only helps when the input data contains large
  /// primitive arrays, otherwise it provides no benefit.
  void SetCacheSize(nsUInt32 uiSizeInKB);

  /// \brief Configures the parser to read from the given stream. This can only be called once on a parser instance.
  void SetInputStream(nsStreamReader& stream, nsUInt32 uiFirstLineOffset = 0); // [tested]

  /// \brief Parses the next portion of the document and triggers appropriate callbacks
  ///
  /// Returns false when the end of the document has been reached or a fatal parsing error occurred.
  /// Use this for incremental parsing where you want to control when parsing happens.
  bool ContinueParsing(); // [tested]

  /// \brief Calls ContinueParsing() in a loop until that returns false.
  nsResult ParseAll(); // [tested]

  /// \brief Skips the rest of the currently open object. No OnEndObject() call will be done for this object either.
  void SkipRestOfObject();

  /// \brief Can be used to prevent parsing the rest of the document.
  void StopParsing();

  /// \brief Outputs that a parsing error was detected (via OnParsingError) and stops further parsing, if bFatal is set to true.
  void ParsingError(nsStringView sMessage, bool bFatal);

  nsLogInterface* m_pLogInterface;

protected:
  /// \brief Called when something unexpected is encountered in the document.
  ///
  /// The error message describes what was expected and what was encountered.
  /// If bFatal is true, the error has left the parser in an unrecoverable state and thus it will not continue parsing.
  /// In that case client code will need to clean up it's open state, as no further callbacks will be called.
  /// If bFatal is false, the document is not entirely valid, but the parser is still able to continue.
  virtual void OnParsingError(nsStringView sMessage, bool bFatal, nsUInt32 uiLine, nsUInt32 uiColumn)
  {
    NS_IGNORE_UNUSED(sMessage);
    NS_IGNORE_UNUSED(bFatal);
    NS_IGNORE_UNUSED(uiLine);
    NS_IGNORE_UNUSED(uiColumn);
  }

  /// \brief Called when a new object is encountered.
  virtual void OnBeginObject(nsStringView sType, nsStringView sName, bool bGlobalName) = 0;

  /// \brief Called when the end of an object is encountered.
  virtual void OnEndObject() = 0;

  /// \brief Called when a new primitive object is encountered.
  virtual void OnBeginPrimitiveList(nsOpenDdlPrimitiveType type, nsStringView sName, bool bGlobalName) = 0;

  /// \brief Called when the end of a primitive object is encountered.
  virtual void OnEndPrimitiveList() = 0;

  /// \todo Currently not supported
  // virtual void OnBeginPrimitiveArrayList(nsOpenDdlPrimitiveType type, nsUInt32 uiGroupSize) = 0;
  // virtual void OnEndPrimitiveArrayList() = 0;

  /// \brief Called when boolean primitive data is available
  ///
  /// Multiple values may be reported at once for efficiency. bThisIsAll indicates if this is the final batch
  /// for the current primitive list. Implementations should accumulate data if bThisIsAll is false.
  virtual void OnPrimitiveBool(nsUInt32 count, const bool* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt8(nsUInt32 count, const nsInt8* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt16(nsUInt32 count, const nsInt16* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt32(nsUInt32 count, const nsInt32* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt64(nsUInt32 count, const nsInt64* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt8(nsUInt32 count, const nsUInt8* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt16(nsUInt32 count, const nsUInt16* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt32(nsUInt32 count, const nsUInt32* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt64(nsUInt32 count, const nsUInt64* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveFloat(nsUInt32 count, const float* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveDouble(nsUInt32 count, const double* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveString(nsUInt32 count, const nsStringView* pData, bool bThisIsAll) = 0;

private:
  enum State
  {
    Finished,
    Idle,
    ReadingBool,
    ReadingInt8,
    ReadingInt16,
    ReadingInt32,
    ReadingInt64,
    ReadingUInt8,
    ReadingUInt16,
    ReadingUInt32,
    ReadingUInt64,
    ReadingFloat,
    ReadingDouble,
    ReadingString,
  };

  struct DdlState
  {
    DdlState()
      : m_State(Idle)
    {
    }
    DdlState(State s)
      : m_State(s)
    {
    }

    State m_State;
  };

  void ReadNextByte();
  bool ReadCharacter();
  bool ReadCharacterSkipComments();
  void SkipWhitespace();
  void ContinueIdle();
  void ReadIdentifier(nsUInt8* szString, nsUInt32& count);
  void ReadString();
  void ReadWord();
  nsUInt64 ReadDecimalLiteral();
  void PurgeCachedPrimitives(bool bThisIsAll);
  bool ContinuePrimitiveList();
  void ContinueString();
  void SkipString();
  void ContinueBool();
  void ContinueInt();
  void ContinueFloat();

  void ReadDecimalFloat();
  void ReadHexString();

  nsHybridArray<DdlState, 32> m_StateStack;
  nsStreamReader* m_pInput;
  nsDynamicArray<nsUInt8> m_Cache;

  static constexpr nsUInt32 s_uiMaxIdentifierLength = 64;

  nsUInt8 m_uiCurByte;
  nsUInt8 m_uiNextByte;
  nsUInt32 m_uiCurLine;
  nsUInt32 m_uiCurColumn;
  bool m_bSkippingMode;
  bool m_bHadFatalParsingError;
  nsUInt8 m_szIdentifierType[s_uiMaxIdentifierLength];
  nsUInt8 m_szIdentifierName[s_uiMaxIdentifierLength];
  nsDynamicArray<nsUInt8> m_TempString;
  nsUInt32 m_uiTempStringLength;

  nsUInt32 m_uiNumCachedPrimitives;
  bool* m_pBoolCache;
  nsInt8* m_pInt8Cache;
  nsInt16* m_pInt16Cache;
  nsInt32* m_pInt32Cache;
  nsInt64* m_pInt64Cache;
  nsUInt8* m_pUInt8Cache;
  nsUInt16* m_pUInt16Cache;
  nsUInt32* m_pUInt32Cache;
  nsUInt64* m_pUInt64Cache;
  float* m_pFloatCache;
  double* m_pDoubleCache;
};
