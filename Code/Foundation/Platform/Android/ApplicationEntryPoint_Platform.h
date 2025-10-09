#pragma once

/// \file
#include <Foundation/Application/Application.h>
#include <Foundation/Platform/Android/Utils/AndroidUtils.h>

class nsApplication;

extern NS_FOUNDATION_DLL void nsAndroidRun(struct android_app* pAndroidApp, nsApplication* pApp);

namespace nsApplicationDetails
{
  template <typename AppClass, typename... Args>
  void EntryFunc(struct android_app* pAndroidApp, Args&&... arguments)
  {
    alignas(alignof(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
    nsAndroidUtils::SetAndroidApp(pAndroidApp);
    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);

    nsAndroidRun(pAndroidApp, pApp);

    pApp->~AppClass();
    memset(pApp, 0, sizeof(AppClass));
  }
} // namespace nsApplicationDetails

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from nsApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define NS_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                        \
  alignas(alignof(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
  NS_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                              \
  extern "C" void android_main(struct android_app* app)                                                                  \
  {                                                                                                                      \
    ::nsApplicationDetails::EntryFunc<AppClass>(app, ##__VA_ARGS__);                                                     \
  }
