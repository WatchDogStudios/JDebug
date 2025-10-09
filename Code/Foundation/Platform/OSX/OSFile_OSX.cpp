#include <Foundation/FoundationPCH.h>

#if NS_ENABLED(NS_PLATFORM_OSX)
#  include <CoreFoundation/CoreFoundation.h>

#  define NS_POSIX_FILE_NOGETAPPLICATIONPATH

#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

nsStringView nsOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    CFBundleRef appBundle = CFBundleGetMainBundle();
    CFURLRef bundleURL = CFBundleCopyBundleURL(appBundle);
    CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);

    if (bundlePath != nullptr)
    {
      CFIndex length = CFStringGetLength(bundlePath);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

      nsArrayPtr<char> temp = NS_DEFAULT_NEW_ARRAY(char, static_cast<nsUInt32>(maxSize));

      if (CFStringGetCString(bundlePath, temp.GetPtr(), maxSize, kCFStringEncodingUTF8))
      {
        s_sApplicationPath = temp.GetPtr();
      }

      NS_DEFAULT_DELETE_ARRAY(temp);
    }

    CFRelease(bundlePath);
    CFRelease(bundleURL);
    CFRelease(appBundle);
  }

  return s_sApplicationPath;
}

#endif
