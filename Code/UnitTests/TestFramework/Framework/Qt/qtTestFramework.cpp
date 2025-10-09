#include <TestFramework/TestFrameworkPCH.h>

#ifdef NS_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
#    include <combaseapi.h>
#  endif

////////////////////////////////////////////////////////////////////////
// nsQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

nsQtTestFramework::nsQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : nsTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
  // Why this is needed: We use the DirectXTex library which calls GetWICFactory to create its factory singleton. If CoUninitialize is called, this pointer is deleted and another access would crash the application. To prevent this, we take the first reference to CoInitializeEx to ensure that nobody else can init+deinit COM and subsequently corrupt the DirectXTex library. As we init+deinit qt for each test, the first test that does an image comparison will init GetWICFactory and the qt deinit would destroy the WICFactory pointer. The next test that uses image comparison would then trigger the crash.
  HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  NS_ASSERT_DEV(SUCCEEDED(res), "CoInitializeEx failed with: {}", nsArgErrorCode(res));
#  endif

  Q_INIT_RESOURCE(resources);
  Initialize();
}

nsQtTestFramework::~nsQtTestFramework()
{
#  if NS_ENABLED(NS_PLATFORM_WINDOWS_DESKTOP)
  CoUninitialize();
#  endif
}


////////////////////////////////////////////////////////////////////////
// nsQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void nsQtTestFramework::OutputImpl(nsTestOutput::Enum Type, const char* szMsg)
{
  nsTestFramework::OutputImpl(Type, szMsg);
}

void nsQtTestFramework::TestResultImpl(nsUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  nsTestFramework::TestResultImpl(uiSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}


void nsQtTestFramework::SetSubTestStatusImpl(nsUInt32 uiSubTestIndex, const char* szStatus)
{
  nsTestFramework::SetSubTestStatusImpl(uiSubTestIndex, szStatus);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}

#endif
