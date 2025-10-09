#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the nsDynamicEnum for a specific type.
class NS_GUIFOUNDATION_DLL nsDynamicStringEnum
{
public:
  /// \brief Returns a nsDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  ///
  /// Calls s_RequestUnknownCallback, if the requested enum is not known yet, which will try to load the data.
  static nsDynamicStringEnum& GetDynamicEnum(nsStringView sEnumName);

  /// \brief Always (re-) creates the nsDynamicEnum under the requested name.
  ///
  /// Use this when you intend to reset the values and don't want them to be loaded from file.
  static nsDynamicStringEnum& CreateDynamicEnum(nsStringView sEnumName);

  /// \brief Removes the entire enum with the given name.
  static void RemoveEnum(nsStringView sEnumName);

  /// \brief Returns all enum values and current names.
  const nsHybridArray<nsString, 16>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void AddValidValue(nsStringView sValue, bool bSortValues = false);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(nsStringView sValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(nsStringView sValue) const;

  /// \brief Sorts existing values alphabetically
  void SortValues();

  /// \brief If set to non-empty, the user can easily edit this enum through a simple dialog and the values will be saved in this file.
  ///
  /// Empty by default, as most dynamic enums need to be set up according to other criteria.
  void SetStorageFile(nsStringView sFile) { m_sStorageFile = sFile; }

  /// \brief The file where values will be stored.
  nsStringView GetStorageFile() const { return m_sStorageFile; }

  /// \brief If specified, the widget shows an "edit" option, which will run nsActionManager::ExecuteAction(sCmd, value)
  ///
  /// This is meant to be used to open existing config dialogs.
  /// There is currently no way to report back a selection, so after making changes, the user has to make another selection.
  void SetEditCommand(nsStringView sCmd, const nsVariant& value);
  nsStringView GetEditCommand() const { return m_sEditCommand; }
  const nsVariant& GetEditCommandValue() const { return m_EditCommandValue; }

  void ReadFromStorage();

  void SaveToStorage();

  /// \brief Invoked by GetDynamicEnum() for enums that are unkonwn at that time.
  ///
  /// Can be used to on-demand load those values, before GetDynamicEnum() returns.
  static nsDelegate<void(nsStringView sEnumName, nsDynamicStringEnum& e)> s_RequestUnknownCallback;

private:
  nsHybridArray<nsString, 16> m_ValidValues;
  nsString m_sStorageFile;
  nsString m_sEditCommand;
  nsVariant m_EditCommandValue;

  static nsMap<nsString, nsDynamicStringEnum> s_DynamicEnums;
};
