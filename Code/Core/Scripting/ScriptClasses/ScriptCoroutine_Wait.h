#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

/// Script coroutine that pauses execution for a specified duration.
///
/// Simple timing coroutine that delays script execution for a given time period.
/// Useful for creating delays in script sequences or implementing timed behaviors.
class NS_CORE_DLL nsScriptCoroutine_Wait : public nsTypedScriptCoroutine<nsScriptCoroutine_Wait, nsTime>
{
public:
  /// Initiates the wait period for the specified duration.
  void Start(nsTime timeout);
  virtual Result Update(nsTime deltaTimeSinceLastUpdate) override;

private:
  nsTime m_TimeRemaing;
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptCoroutine_Wait);
