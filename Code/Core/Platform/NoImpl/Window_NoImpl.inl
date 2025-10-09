#include <Core/System/Window.h>

#include <Core/Platform/NoImpl/Window_NoImpl.h>

nsWindowNoImpl::~nsWindowNoImpl()
{
}

nsResult nsWindowNoImpl::InitializeWindow()
{
  NS_ASSERT_NOT_IMPLEMENTED;
  return NS_FAILURE;
}

void nsWindowNoImpl::DestroyWindow()
{
  NS_ASSERT_NOT_IMPLEMENTED;
}

nsResult nsWindowNoImpl::Resize(const nsSizeU32& newWindowSize)
{
  NS_ASSERT_NOT_IMPLEMENTED;
  return NS_FAILURE;
}

void nsWindowNoImpl::ProcessWindowMessages()
{
  NS_ASSERT_NOT_IMPLEMENTED;
}

void nsWindowNoImpl::OnResize(const nsSizeU32& newWindowSize)
{
  NS_ASSERT_NOT_IMPLEMENTED;
}

nsWindowHandle nsWindowNoImpl::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
