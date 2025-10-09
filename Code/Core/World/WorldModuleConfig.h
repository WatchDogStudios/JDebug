#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief Configuration class for world module interface implementations.
///
/// Manages the mapping between world module interfaces and their specific implementations.
/// This is used when multiple implementations exist for the same interface, allowing
/// configuration of which implementation should be used by default.
class NS_CORE_DLL nsWorldModuleConfig
{
public:
  nsResult Save();
  void Load();

  /// \brief Applies the current configuration to the world module factory.
  void Apply();

  /// \brief Adds a mapping from an interface to a specific implementation.
  void AddInterfaceImplementation(nsStringView sInterfaceName, nsStringView sImplementationName);

  /// \brief Removes the implementation mapping for the given interface.
  void RemoveInterfaceImplementation(nsStringView sInterfaceName);

  /// \brief Represents a mapping between an interface and its implementation.
  struct InterfaceImpl
  {
    nsString m_sInterfaceName;      ///< Name of the world module interface
    nsString m_sImplementationName; ///< Name of the specific implementation to use

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  nsHybridArray<InterfaceImpl, 8> m_InterfaceImpls; ///< List of interface to implementation mappings
};
