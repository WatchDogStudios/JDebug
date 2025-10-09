#include <Core/CorePCH.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Configuration/Startup.h>

NS_IMPLEMENT_SINGLETON(nsWindowManager);

//////////////////////////////////////////////////////////////////////////

static nsUniquePtr<nsWindowManager> s_pWindowManager;

// clang-format off
NS_BEGIN_SUBSYSTEM_DECLARATION(Core, nsWindowManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pWindowManager = NS_DEFAULT_NEW(nsWindowManager);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pWindowManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pWindowManager)
    {
      s_pWindowManager->CloseAll(nullptr);
    }
  }

NS_END_SUBSYSTEM_DECLARATION;

// clang-format on

//////////////////////////////////////////////////////////////////////////

nsWindowManager::nsWindowManager()
  : m_SingletonRegistrar(this)
{
}

nsWindowManager::~nsWindowManager()
{
  CloseAll(nullptr);
}

void nsWindowManager::Update()
{
  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pWindow->ProcessWindowMessages();
  }
}

void nsWindowManager::Close(nsRegisteredWndHandle hWindow)
{
  nsUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  Data* pData = pDataPtr->Borrow();
  NS_ASSERT_DEV(pData != nullptr, "Invalid window data");

  if (pData->m_OnDestroy.IsValid())
  {
    pData->m_OnDestroy(hWindow);
  }

  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.

  if (pData->m_pOutputTarget)
  {
    pData->m_pOutputTarget.Clear();
  }

  if (pData->m_pWindow)
  {
    pData->m_pWindow.Clear();
  }

  m_Data.Remove(hWindow.GetInternalID());
}

void nsWindowManager::CloseAll(const void* pCreatedBy)
{
  nsDynamicArray<nsRegisteredWndHandle> toClose;

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      toClose.PushBack(nsRegisteredWndHandle(it.Id()));
    }
  }

  for (const nsRegisteredWndHandle& hWindow : toClose)
  {
    Close(hWindow);
  }
}

bool nsWindowManager::IsValid(nsRegisteredWndHandle hWindow) const
{
  return m_Data.Contains(hWindow.GetInternalID());
}

void nsWindowManager::GetRegistered(nsDynamicArray<nsRegisteredWndHandle>& out_windowHandles, const void* pCreatedBy /*= nullptr*/)
{
  out_windowHandles.Clear();

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      out_windowHandles.PushBack(nsRegisteredWndHandle(it.Id()));
    }
  }
}

nsRegisteredWndHandle nsWindowManager::Register(nsStringView sName, const void* pCreatedBy, nsUniquePtr<nsWindowBase>&& pWindow)
{
  NS_ASSERT_ALWAYS(pCreatedBy != nullptr, "pCreatedBy is invalid");
  NS_ASSERT_ALWAYS(pWindow != nullptr, "pWindow is invalid");

  nsUniquePtr<Data> pData = NS_DEFAULT_NEW(Data);
  pData->m_sName = sName;
  pData->m_pCreatedBy = pCreatedBy;
  pData->m_pWindow = std::move(pWindow);

  return nsRegisteredWndHandle(m_Data.Insert(std::move(pData)));
}

void nsWindowManager::SetOutputTarget(nsRegisteredWndHandle hWindow, nsUniquePtr<nsWindowOutputTargetBase>&& pOutputTarget)
{
  nsUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_pOutputTarget = std::move(pOutputTarget);
}

void nsWindowManager::SetDestroyCallback(nsRegisteredWndHandle hWindow, nsWindowDestroyFunc onDestroyCallback)
{
  nsUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_OnDestroy = onDestroyCallback;
}

nsStringView nsWindowManager::GetName(nsRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nsStringView();

  return m_Data[hWindow.GetInternalID()]->m_sName;
}

nsWindowBase* nsWindowManager::GetWindow(nsRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pWindow.Borrow();
}

nsWindowOutputTargetBase* nsWindowManager::GetOutputTarget(nsRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pOutputTarget.Borrow();
}


NS_STATICLINK_FILE(Core, Core_System_Implementation_WindowManager);
