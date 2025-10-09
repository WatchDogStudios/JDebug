#pragma once

#include <Foundation/Basics.h>

#ifndef NS_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#  define NS_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#endif

/// \brief Macro to define the application entry point for all test applications
#define NS_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
  NS_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION                                             \
  NS_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
  int main(int argc, char** argv)                                                         \
  {                                                                                       \
    nsTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define NS_TESTFRAMEWORK_ENTRY_POINT_END()                        \
  while (nsTestSetup::RunTests() == nsTestAppRun::Continue)       \
  {                                                               \
  }                                                               \
  const nsInt32 iFailedTests = nsTestSetup::GetFailedTestCount(); \
  nsTestSetup::DeInitTestFramework();                             \
  return iFailedTests;                                            \
  }

#define NS_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  NS_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  NS_TESTFRAMEWORK_ENTRY_POINT_END()
