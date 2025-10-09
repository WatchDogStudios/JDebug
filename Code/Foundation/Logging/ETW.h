#pragma once

#include <Foundation/Logging/Log.h>

/// \brief A simple log writer that outputs all log messages to the ns ETW provider.
class NS_FOUNDATION_DLL nsETW
{
public:
  /// \brief Register this at nsLog to write all log messages to ETW.
  static void LogMessageHandler(const nsLoggingEventData& eventData)
  {
    nsETW::LogMessage(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
  }

  /// \brief Log Message to ETW.
  static void LogMessage(nsLogMsgType::Enum eventType, nsUInt8 uiIndentation, nsStringView sText);
};
