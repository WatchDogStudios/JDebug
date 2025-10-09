
#pragma once

#include <Foundation/IO/Stream.h>

/// \brief A stream reader that wraps another stream to track how many bytes are read from it.
class NS_FOUNDATION_DLL nsStreamReaderWithStats : public nsStreamReader
{
public:
  nsStreamReaderWithStats() = default;
  nsStreamReaderWithStats(nsStreamReader* pStream)
    : m_pStream(pStream)
  {
  }

  virtual nsUInt64 ReadBytes(void* pReadBuffer, nsUInt64 uiBytesToRead) override
  {
    const nsUInt64 uiRead = m_pStream->ReadBytes(pReadBuffer, uiBytesToRead);
    m_uiBytesRead += uiRead;
    return uiRead;
  }

  nsUInt64 SkipBytes(nsUInt64 uiBytesToSkip) override
  {
    const nsUInt64 uiSkipped = m_pStream->SkipBytes(uiBytesToSkip);
    m_uiBytesSkipped += uiSkipped;
    return uiSkipped;
  }

  /// the stream to forward all requests to
  nsStreamReader* m_pStream = nullptr;

  /// the number of bytes that were read from the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  nsUInt64 m_uiBytesRead = 0;

  /// the number of bytes that were skipped from the wrapped stream
  nsUInt64 m_uiBytesSkipped = 0;
};

/// \brief A stream writer that wraps another stream to track how many bytes are written to it.
class NS_FOUNDATION_DLL nsStreamWriterWithStats : public nsStreamWriter
{
public:
  nsStreamWriterWithStats() = default;
  nsStreamWriterWithStats(nsStreamWriter* pStream)
    : m_pStream(pStream)
  {
  }

  virtual nsResult WriteBytes(const void* pWriteBuffer, nsUInt64 uiBytesToWrite) override
  {
    m_uiBytesWritten += uiBytesToWrite;
    return m_pStream->WriteBytes(pWriteBuffer, uiBytesToWrite);
  }

  nsResult Flush() override
  {
    return m_pStream->Flush();
  }

  /// the stream to forward all requests to
  nsStreamWriter* m_pStream = nullptr;

  /// the number of bytes that were written to the wrapped stream
  /// public access so that users can read and modify this in case they want to reset the value at any time
  nsUInt64 m_uiBytesWritten = 0;
};
