#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Time/Timestamp.h>

#include <ConsoleWriter_Platform.inl>

nsLog::TimestampMode nsLogWriter::Console::s_TimestampMode = nsLog::TimestampMode::None;

void nsLogWriter::Console::LogMessageHandler(const nsLoggingEventData& eventData)
{
  nsStringBuilder sTimestamp;
  nsLog::GenerateFormattedTimestamp(s_TimestampMode, sTimestamp);

  static nsMutex WriterLock; // will only be created if this writer is used at all
  NS_LOCK(WriterLock);

  if (eventData.m_EventType == nsLogMsgType::BeginGroup)
    printf("\n");

  nsHybridArray<char, 11> indentation;
  indentation.SetCount(eventData.m_uiIndentation + 1, ' ');
  indentation[eventData.m_uiIndentation] = 0;

  nsStringBuilder sTemp1, sTemp2;

  switch (eventData.m_EventType)
  {
    case nsLogMsgType::Flush:
      fflush(stdout);
      break;

    case nsLogMsgType::BeginGroup:
      SetConsoleColor(0x02);
      printf("%s+++++ %s (%s) +++++\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_sTag.GetData(sTemp2));
      break;

    case nsLogMsgType::EndGroup:
      SetConsoleColor(0x02);
#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT)
      printf("%s----- %s (%.6f sec)-----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_fSeconds);
#else
      printf("%s----- %s (%s)-----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), "timing info not available");
#endif
      break;

    case nsLogMsgType::ErrorMsg:
      SetConsoleColor(0x0C);
      printf("%s%sError: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case nsLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("%s%sSeriously: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case nsLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("%s%sWarning: %s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case nsLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      fflush(stdout);
      break;

    case nsLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case nsLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    case nsLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));
      break;

    default:
      SetConsoleColor(0x0D);
      printf("%s%s%s\n", indentation.GetData(), sTimestamp.GetData(), eventData.m_sText.GetData(sTemp1));

      nsLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}

void nsLogWriter::Console::SetTimestampMode(nsLog::TimestampMode mode)
{
  s_TimestampMode = mode;
}
