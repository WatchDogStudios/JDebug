#pragma once

#include <Core/Console/CommandInterpreter.h>
#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

/// \brief The event data that is broadcast by the console
struct nsConsoleEvent
{
  enum class Type : nsInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// \brief The console string that was just added.
  const nsConsoleString* m_AddedpConsoleString;
};

/// \brief Base console system for command input, output display, and history management.
///
/// Provides infrastructure for command execution through pluggable interpreters,
/// maintains input history, and broadcasts events when output is added.
/// Thread-safe through internal mutex protection.
///
/// This base class handles core functionality but doesn't store output strings.
/// Derived classes typically provide persistent storage and visual representation.
class NS_CORE_DLL nsConsole
{
public:
  nsConsole();
  virtual ~nsConsole();

  /// \name Events
  /// @{

public:
  /// \brief Grants access to subscribe and unsubscribe from console events.
  const nsEvent<const nsConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// \brief The console event variable, to attach to.
  nsEvent<const nsConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// \brief Returns the mutex that's used to prevent multi-threaded access
  nsMutex& GetMutex() const { return m_Mutex; }

  static void SetMainConsole(nsConsole* pConsole);
  static nsConsole* GetMainConsole();

protected:
  mutable nsMutex m_Mutex;

private:
  static nsConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// \brief Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const nsSharedPtr<nsCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// \brief Returns the currently used command interpreter.
  const nsSharedPtr<nsCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// \brief Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(nsStringBuilder& ref_sText);

  /// \brief Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(nsStringView sInput);

protected:
  nsSharedPtr<nsCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// \brief Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(nsStringView sText, nsConsoleString::Type type = nsConsoleString::Type::Default);

  /// \brief Display the console state.
  virtual void RenderConsole() {}

  /// @}

  /// \name Input
  /// @{

public:
  /// \brief Update the console with the latest input.
  virtual void HandleInput() {}

  /// \brief Adds an item to the input history.
  void AddToInputHistory(nsStringView sText);

  /// \brief Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const nsStaticArray<nsString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(nsInt32 iHistoryUp, nsStringBuilder& ref_sResult);

  /// \brief Writes the current input history to a text file.
  nsResult SaveInputHistory(nsStringView sFile);

  /// \brief Reads the text file and appends all lines to the input history.
  void LoadInputHistory(nsStringView sFile);

protected:
  nsInt32 m_iCurrentInputHistoryElement = -1;
  nsStaticArray<nsString, 16> m_InputHistory;

  /// @}
};
