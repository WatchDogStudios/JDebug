#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing access to console variables (CVars) from scripts.
///
/// Allows scripts to read and modify CVars for configuration and debugging purposes.
/// Provides type-safe accessors for common CVar types as well as generic variant access.
class NS_CORE_DLL nsScriptExtensionClass_CVar
{
public:
  static nsVariant GetValue(nsStringView sName);
  static bool GetBoolValue(nsStringView sName);
  static int GetIntValue(nsStringView sName);
  static float GetFloatValue(nsStringView sName);
  static nsString GetStringValue(nsStringView sName);

  static void SetValue(nsStringView sName, const nsVariant& value);
  static void SetBoolValue(nsStringView sName, bool bValue);
  static void SetIntValue(nsStringView sName, int iValue);
  static void SetFloatValue(nsStringView sName, float fValue);
  static void SetStringValue(nsStringView sName, const nsString& sValue);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_CVar);
