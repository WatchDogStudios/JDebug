#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class nsLogInterface;

/// \brief An nsResult with an additional message for the reason of failure
struct [[nodiscard]] NS_FOUNDATION_DLL nsStatus
{
  /// \brief Sets the status to NS_FAILURE and stores the error message.
  explicit nsStatus(const char* szError)
    : m_Result(NS_FAILURE)
    , m_sMessage(szError)
  {
  }

  /// \brief Sets the status to NS_FAILURE and stores the error message.
  explicit nsStatus(nsStringView sError)
    : m_Result(NS_FAILURE)
    , m_sMessage(sError)
  {
  }

  /// \brief Sets the status, but doesn't store a message string.
  NS_ALWAYS_INLINE nsStatus(nsResult r)
    : m_Result(r)
  {
  }

  /// \brief Sets the status, but doesn't store a message string.
  NS_ALWAYS_INLINE nsStatus(nsResultEnum r)
    : m_Result(r)
  {
  }

  /// \brief Sets the status to NS_FAILURE and stores the error message. Can be used with nsFmt().
  explicit nsStatus(const nsFormatString& fmt);

  [[nodiscard]] nsResult GetResult() const { return m_Result; }

  [[nodiscard]] NS_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] NS_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  NS_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief If the state is NS_FAILURE, the message is written to the given log (or the currently active thread-local log).
  ///
  /// The return value is the same as 'Failed()' but isn't marked as [[nodiscard]], ie returns true, if a failure happened,
  /// so can be used in a conditional.
  bool LogFailure(nsLogInterface* pLog = nullptr) const;

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a szMsg is given, this will be the assert message.
  /// Additionally m_sMessage will be included as a detailed message.
  void AssertSuccess(const char* szMsg = nullptr) const;

  [[nodiscard]] const nsString& GetMessageString() const { return m_sMessage; }

private:
  nsResult m_Result;
  nsString m_sMessage;
};

NS_ALWAYS_INLINE nsResult nsToResult(const nsStatus& result)
{
  return result.GetResult();
}
