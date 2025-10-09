#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Logging/Log.h>

void nsFileserverApp::FileserverEventHandlerConsole(const nsFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case nsFileserverEvent::Type::None:
      nsLog::Error("Invalid Fileserver event type");
      break;

    case nsFileserverEvent::Type::ServerStarted:
    {
      nsLog::Info("nsFileserver is running");
    }
    break;

    case nsFileserverEvent::Type::ServerStopped:
    {
      nsLog::Info("nsFileserver was shut down");
    }
    break;

    case nsFileserverEvent::Type::ClientConnected:
    {
      nsLog::Success("Client connected");
    }
    break;

    case nsFileserverEvent::Type::MountDataDir:
    {
      nsLog::Info("Mounted data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case nsFileserverEvent::Type::UnmountDataDir:
    {
      nsLog::Info("Unmount request for data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case nsFileserverEvent::Type::FileDownloadRequest:
    {
      if (e.m_FileState == nsFileserveFileState::NonExistant)
        nsLog::Dev("Request: (N/A) '{0}'", e.m_szPath);

      if (e.m_FileState == nsFileserveFileState::SameHash)
        nsLog::Dev("Request: (HASH) '{0}'", e.m_szPath);

      if (e.m_FileState == nsFileserveFileState::SameTimestamp)
        nsLog::Dev("Request: (TIME) '{0}'", e.m_szPath);

      if (e.m_FileState == nsFileserveFileState::NonExistantEither)
        nsLog::Dev("Request: (N/AE) '{0}'", e.m_szPath);

      if (e.m_FileState == nsFileserveFileState::Different)
        nsLog::Info("Request: '{0}' ({1} bytes)", e.m_szPath, e.m_uiSizeTotal);
    }
    break;

    case nsFileserverEvent::Type::FileDownloading:
    {
      nsLog::Debug("Transfer: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    }
    break;

    case nsFileserverEvent::Type::FileDownloadFinished:
    {
      if (e.m_FileState == nsFileserveFileState::Different)
        nsLog::Info("Transfer done.");
    }
    break;

    case nsFileserverEvent::Type::FileDeleteRequest:
    {
      nsLog::Warning("File Deletion: '{0}'", e.m_szPath);
    }
    break;

    case nsFileserverEvent::Type::FileUploading:
      nsLog::Debug("Upload: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
      break;

    case nsFileserverEvent::Type::FileUploadFinished:
      nsLog::Info("Upload finished: {0}", e.m_szPath);
      break;

    default:
      break;
  }
}
