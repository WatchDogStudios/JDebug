#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <ConsoleOutput_Platform.inl>

void OutputToConsole(nsTestOutput::Enum type, const char* szMsg)
{
  static nsInt32 iIndentation = 0;
  static bool bAnyError = false;

  nsUInt8 uiColor = 0x07;

  switch (type)
  {
    case nsTestOutput::StartOutput:
      break;
    case nsTestOutput::BeginBlock:
      iIndentation += 2;
      break;
    case nsTestOutput::EndBlock:
      iIndentation -= 2;
      break;
    case nsTestOutput::Details:
      break;
    case nsTestOutput::ImportantInfo:
      break;
    case nsTestOutput::Success:
      uiColor = 0x0A;
      break;
    case nsTestOutput::Message:
      uiColor = 0x0E;
      break;
    case nsTestOutput::Warning:
      uiColor = 0x0C;
      break;
    case nsTestOutput::Error:
      uiColor = 0x0C;
      bAnyError = true;
      break;
    case nsTestOutput::Duration:
    case nsTestOutput::ImageDiffFile:
    case nsTestOutput::InvalidType:
    case nsTestOutput::AllOutputTypes:
      return;

    case nsTestOutput::FinalResult:
      if (bAnyError)
        uiColor = 0x0C;
      else
        uiColor = 0x0A;

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      NS_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  OutputToConsole_Platform(uiColor, type, iIndentation, szMsg);

  if (type >= nsTestOutput::Error)
  {
    fflush(stdout);
  }
}
