#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/RefCounted.h>

struct NS_CORE_DLL nsConsoleString
{
  enum class Type : nsUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type m_Type = Type::Default;
  nsString m_sText;
  nsColor GetColor() const;

  bool operator<(const nsConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct NS_CORE_DLL nsCommandInterpreterState
{
  nsStringBuilder m_sInput;
  nsHybridArray<nsConsoleString, 16> m_sOutput;

  void AddOutputLine(const nsFormatString& text, nsConsoleString::Type type = nsConsoleString::Type::Default);
};

class NS_CORE_DLL nsCommandInterpreter : public nsRefCounted
{
public:
  virtual void Interpret(nsCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(nsCommandInterpreterState& inout_state);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(nsStringView sVariable, nsDeque<nsString>& ref_commonStrings, nsDeque<nsConsoleString>& ref_consoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(nsStringView sVariable, nsDeque<nsString>& ref_commonStrings, nsDeque<nsConsoleString>& ref_consoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const nsString FindCommonString(const nsDeque<nsString>& strings);

  /// \name Helpers
  /// @{

  /// \brief Returns a nice string containing all the important information about the cvar.
  static nsString GetFullInfoAsString(nsCVar* pCVar);

  /// \brief Returns the value of the cvar as a string.
  static const nsString GetValueAsString(nsCVar* pCVar);

  /// @}
};
