#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_ANDROID)
#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

#  define NS_POSIX_FILE_NOGETAPPLICATIONPATH
#  define NS_POSIX_FILE_NOGETUSERDATAFOLDER
#  define NS_POSIX_FILE_NOGETTEMPDATAFOLDER
#  define NS_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

nsStringView nsOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    nsJniAttachment attachment;

    nsJniString packagePath = attachment.GetActivity().Call<nsJniString>("getPackageCodePath");
    // By convention, android requires assets to be placed in the 'Assets' folder
    // inside the apk thus we use that as our SDK root.
    nsStringBuilder sTemp = packagePath.GetData();
    sTemp.AppendPath("Assets/nsDummyBin");
    s_sApplicationPath = sTemp;
  }

  return s_sApplicationPath;
}

nsString nsOSFile::GetUserDataFolder(nsStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    android_app* app = nsAndroidUtils::GetAndroidApp();
    s_sUserDataPath = app->activity->internalDataPath;
  }

  nsStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

nsString nsOSFile::GetTempDataFolder(nsStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    nsJniAttachment attachment;

    nsJniObject cacheDir = attachment.GetActivity().Call<nsJniObject>("getCacheDir");
    nsJniString path = cacheDir.Call<nsJniString>("getPath");
    s_sTempDataPath = path.GetData();
  }

  nsStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

nsString nsOSFile::GetUserDocumentsFolder(nsStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    NS_ASSERT_NOT_IMPLEMENTED;
  }

  nsStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif
