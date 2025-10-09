#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing logging functionality from scripts.
///
/// Allows scripts to output formatted log messages at different severity levels.
/// Messages are sent to the standard WDFramework logging system and will appear
/// in the console, log files, and other registered log writers.
class NS_CORE_DLL nsScriptExtensionClass_Log
{
public:
  static void Info(nsStringView sText, const nsVariantArray& params);
  static void Warning(nsStringView sText, const nsVariantArray& params);
  static void Error(nsStringView sText, const nsVariantArray& params);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_Log);
