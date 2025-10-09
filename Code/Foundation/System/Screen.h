#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

struct nsScreenResolution
{
  NS_DECLARE_POD_TYPE();

  nsUInt32 m_uiResolutionX = 0;
  nsUInt32 m_uiResolutionY = 0;
  nsUInt16 m_uiRefreshRate = 0;
  nsUInt8 m_uiBitsPerPixel = 0;

  inline bool operator<(const nsScreenResolution& rhs) const
  {
    if (m_uiBitsPerPixel != rhs.m_uiBitsPerPixel)
      return m_uiBitsPerPixel < rhs.m_uiBitsPerPixel;

    if (m_uiRefreshRate != rhs.m_uiRefreshRate)
      return m_uiRefreshRate < rhs.m_uiRefreshRate;

    if (m_uiResolutionX != rhs.m_uiResolutionX)
      return m_uiResolutionX < rhs.m_uiResolutionX;

    return m_uiResolutionY < rhs.m_uiResolutionY;
  }
};

/// \brief Describes the properties of a screen
struct NS_FOUNDATION_DLL nsScreenInfo
{
  nsString m_sDisplayID;   ///< Internal name used by the OS to identify the monitor.
  nsString m_sDisplayName; ///< Some OS provided name for the screen, typically the manufacturer and model name.

  nsInt32 m_iOffsetX;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  nsInt32 m_iOffsetY;      ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  nsInt32 m_iResolutionX;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  nsInt32 m_iResolutionY;  ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool m_bIsPrimary;       ///< Whether this is the primary/main screen.

  nsDynamicArray<nsScreenResolution> m_SupportedResolutions;
};

/// \brief Provides functionality to detect available monitors
class NS_FOUNDATION_DLL nsScreen
{
public:
  /// \brief Enumerates all available screens. When it returns NS_SUCCESS, at least one screen has been found.
  static nsResult EnumerateScreens(nsDynamicArray<nsScreenInfo>& out_screens);

  /// \brief Prints the available screen information to the provided log.
  static void PrintScreenInfo(const nsHybridArray<nsScreenInfo, 2>& screens, nsLogInterface* pLog = nsLog::GetThreadLocalLogSystem());
};
