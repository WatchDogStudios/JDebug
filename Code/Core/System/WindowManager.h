#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>

class nsWindowBase;
class nsWindowOutputTargetBase;

using nsRegisteredWndHandleData = nsGenericId<16, 16>;

/// \brief Handle type for windows registered with the nsWindowManager.
///
/// Default-constructed handles are invalid and can be checked with IsInvalidated().
/// This handle type is separate from native platform window handles (nsWindowHandle).
class nsRegisteredWndHandle
{
  NS_DECLARE_HANDLE_TYPE(nsRegisteredWndHandle, nsRegisteredWndHandleData);
};

/// \brief Callback function type called when a registered window is destroyed.
using nsWindowDestroyFunc = nsDelegate<void(nsRegisteredWndHandle)>;

/// \brief Manages registered windows and their associated data.
///
/// The WindowManager provides a centralized system for managing windows throughout
/// their lifetime. Windows are registered with unique handles and can have associated
/// output targets and destruction callbacks.
class NS_CORE_DLL nsWindowManager final
{
  NS_DECLARE_SINGLETON(nsWindowManager);

public:
  nsWindowManager();
  ~nsWindowManager();

  /// \brief Processes window messages for all registered windows.
  ///
  /// This should be called regularly (typically once per frame) to handle
  /// platform-specific window events.
  void Update();

  /// \brief Registers a new window with the manager.
  ///
  /// \param sName Human-readable name for the window (for debugging)
  /// \param pCreatedBy Pointer identifying the creator (used for bulk operations)
  /// \param pWindow The window implementation to register
  /// \return Handle to the registered window
  ///
  /// The returned handle remains valid until the window is explicitly closed.
  /// The pCreatedBy parameter allows closing all windows created by a specific object.
  nsRegisteredWndHandle Register(nsStringView sName, const void* pCreatedBy, nsUniquePtr<nsWindowBase>&& pWindow);

  /// \brief Retrieves handles for all registered windows.
  ///
  /// \param out_WindowIDs Array to fill with window handles
  /// \param pCreatedBy Optional filter to only return windows created by this object
  void GetRegistered(nsDynamicArray<nsRegisteredWndHandle>& out_windowHandles, const void* pCreatedBy = nullptr);

  /// \brief Checks if a window handle is valid and refers to an existing window.
  ///
  /// Invalid handles can occur if the window was closed or if using a default-constructed handle.
  bool IsValid(nsRegisteredWndHandle hWindow) const;

  /// \brief Gets the name of a registered window.
  nsStringView GetName(nsRegisteredWndHandle hWindow) const;

  /// \brief Gets the window implementation for a registered window.
  nsWindowBase* GetWindow(nsRegisteredWndHandle hWindow) const;

  /// \brief Sets a callback to be invoked when the window is destroyed.
  ///
  /// The callback receives the window handle as parameter. Only one callback
  /// can be set per window; setting a new callback replaces the previous one.
  void SetDestroyCallback(nsRegisteredWndHandle hWindow, nsWindowDestroyFunc onDestroyCallback);

  /// \brief Associates an output target with a registered window.
  ///
  /// Output targets are destroyed before the window to ensure proper cleanup order.
  /// Setting a new output target replaces any existing one.
  void SetOutputTarget(nsRegisteredWndHandle hWindow, nsUniquePtr<nsWindowOutputTargetBase>&& pOutputTarget);

  /// \brief Gets the output target associated with a window.
  nsWindowOutputTargetBase* GetOutputTarget(nsRegisteredWndHandle hWindow) const;

  /// \brief Closes and unregisters a specific window.
  ///
  /// This first calls any registered destroy callback, then destroys the output target, then the window.
  /// The handle becomes invalid after this call.
  void Close(nsRegisteredWndHandle hWindow);

  /// \brief Closes all windows created by a specific object.
  ///
  /// \param pCreatedBy Identifier of the creator, or nullptr to close all windows
  ///
  /// This is useful for cleanup when an object that created multiple windows is destroyed.
  void CloseAll(const void* pCreatedBy);

private:
  struct Data
  {
    nsString m_sName;
    const void* m_pCreatedBy = nullptr;
    nsUniquePtr<nsWindowBase> m_pWindow;
    nsUniquePtr<nsWindowOutputTargetBase> m_pOutputTarget;
    nsWindowDestroyFunc m_OnDestroy;
  };

  nsIdTable<nsRegisteredWndHandleData, nsUniquePtr<Data>> m_Data;
};
