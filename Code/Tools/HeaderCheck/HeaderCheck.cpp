#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/LinearAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>


namespace
{
  NS_ALWAYS_INLINE void SkipWhitespace(nsToken& ref_token, nsUInt32& i, const nsDeque<nsToken>& tokens)
  {
    while (ref_token.m_iType == nsTokenType::Whitespace)
    {
      ref_token = tokens[++i];
    }
  }

  NS_ALWAYS_INLINE void SkipLine(nsToken& ref_token, nsUInt32& i, const nsDeque<nsToken>& tokens)
  {
    while (ref_token.m_iType != nsTokenType::Newline && ref_token.m_iType != nsTokenType::EndOfFile)
    {
      ref_token = tokens[++i];
    }
  }
} // namespace

class nsHeaderCheckApp : public nsApplication
{
private:
  nsString m_sSearchDir;
  nsString m_sProjectName;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  nsUniquePtr<nsLinearAllocator<nsAllocatorTrackingMode::Nothing>> m_pStackAllocator;
  nsDynamicArray<nsString> m_IncludeDirectories;

  struct IgnoreInfo
  {
    nsHashSet<nsString> m_byName;
  };

  IgnoreInfo m_IgnoreTarget;
  IgnoreInfo m_IgnoreSource;

public:
  using SUPER = nsApplication;

  nsHeaderCheckApp()
    : nsApplication("HeaderCheck")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const nsLoggingEventData& eventData)
  {
    nsHeaderCheckApp* app = (nsHeaderCheckApp*)nsApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case nsLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case nsLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case nsLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  nsResult ParseArray(const nsVariant& value, nsHashSet<nsString>& ref_dst)
  {
    if (!value.CanConvertTo<nsVariantArray>())
    {
      nsLog::Error("Expected array");
      return NS_FAILURE;
    }
    auto a = value.Get<nsVariantArray>();
    const auto arraySize = a.GetCount();
    for (nsUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<nsString>())
      {
        nsLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return NS_FAILURE;
      }
      nsStringBuilder file = el.Get<nsString>();
      file.ToLower();
      ref_dst.Insert(file);
    }
    return NS_SUCCESS;
  }

  nsResult ParseIgnoreFile(const nsStringView sIgnoreFilePath)
  {
    nsJSONReader jsonReader;
    jsonReader.SetLogInterface(nsLog::GetThreadLocalLogSystem());

    nsFileReader reader;
    if (reader.Open(sIgnoreFilePath).Failed())
    {
      nsLog::Error("Failed to open ignore file {0}", sIgnoreFilePath);
      return NS_FAILURE;
    }

    if (jsonReader.Parse(reader).Failed())
      return NS_FAILURE;

    const nsStringView includeTarget = "includeTarget";
    const nsStringView includeSource = "includeSource";
    const nsStringView byName = "byName";

    if (jsonReader.GetTopLevelElementType() != nsJSONReader::ElementType::Dictionary)
    {
      nsLog::Error("Ignore file {0} does not start with a json object", sIgnoreFilePath);
      return NS_FAILURE;
    }

    auto topLevel = jsonReader.GetTopLevelObject();
    for (auto it = topLevel.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Key() == includeTarget || it.Key() == includeSource)
      {
        IgnoreInfo& info = (it.Key() == includeTarget) ? m_IgnoreTarget : m_IgnoreSource;
        auto inner = it.Value().Get<nsVariantDictionary>();
        for (auto it2 = inner.GetIterator(); it2.IsValid(); it2.Next())
        {
          if (it2.Key() == byName)
          {
            if (ParseArray(it2.Value(), info.m_byName).Failed())
            {
              nsLog::Error("Failed to parse value of '{0}.{1}'.", it.Key(), it2.Key());
              return NS_FAILURE;
            }
          }
          else
          {
            nsLog::Error("Unknown field of '{0}.{1}'", it.Key(), it2.Key());
            return NS_FAILURE;
          }
        }
      }
      else
      {
        nsLog::Error("Unknown json member in root object '{0}'", it.Key().GetView());
        return NS_FAILURE;
      }
    }
    return NS_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
    nsGlobalLog::AddLogWriter(LogInspector);

    m_pStackAllocator = NS_DEFAULT_NEW(nsLinearAllocator<nsAllocatorTrackingMode::Nothing>, "Temp Allocator", nsFoundation::GetAlignedAllocator());

    if (GetArgumentCount() < 2)
      nsLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    nsFileSystem::AddDataDirectory("", "App", ":", nsDataDirUsage::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    nsStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    auto shortInclude = nsStringView("-i");
    auto longInclude = nsStringView("--includeDir");
    auto shortIgnoreFile = nsStringView("-f");
    auto longIgnoreFile = nsStringView("--ignoreFile");
    for (nsUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = nsStringView(GetArgument(argi));
      if (arg == shortInclude || arg == longInclude)
      {
        if (numArgs <= argi + 1)
        {
          nsLog::Error("Missing path for {0}", arg);
          return;
        }
        nsStringBuilder includeDir = GetArgument(argi + 1);
        if (includeDir == shortInclude || includeDir == longInclude || includeDir == shortIgnoreFile || includeDir == longIgnoreFile)
        {
          nsLog::Error("Missing path for {0} found {1} instead", arg, includeDir.GetView());
          return;
        }
        argi++;
        includeDir.MakeCleanPath();
        m_IncludeDirectories.PushBack(includeDir);
      }
      else if (arg == shortIgnoreFile || arg == longIgnoreFile)
      {
        if (numArgs <= argi + 1)
        {
          nsLog::Error("Missing path for {0}", arg);
          return;
        }
        nsStringBuilder ignoreFile = GetArgument(argi + 1);
        if (ignoreFile == shortInclude || ignoreFile == longInclude || ignoreFile == shortIgnoreFile || ignoreFile == longIgnoreFile)
        {
          nsLog::Error("Missing path for {0} found {1} instead", arg, ignoreFile.GetView());
          return;
        }
        argi++;
        ignoreFile.MakeCleanPath();
        if (ParseIgnoreFile(ignoreFile.GetView()).Failed())
          return;
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          nsLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir?");
        }
      }
    }

    if (!nsPathUtils::IsAbsolutePath(sSearchDir.GetData()))
      nsLog::Error("The given path is not absolute: '{0}'", sSearchDir);

    m_sSearchDir = sSearchDir;

    auto projectStart = m_sSearchDir.GetView().FindLastSubString("/");
    if (projectStart == nullptr)
    {
      nsLog::Error("Failed to parse project name from search path {0}", sSearchDir);
      return;
    }
    nsStringBuilder projectName = nsStringView(projectStart + 1, m_sSearchDir.GetView().GetEndPointer());
    projectName.ToUpper();
    m_sProjectName = projectName;

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      nsLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    m_pStackAllocator = nullptr;

    nsGlobalLog::RemoveLogWriter(LogInspector);
    nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
  }

  nsResult ReadEntireFile(nsStringView sFile, nsStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    nsFileReader File;
    if (File.Open(sFile) == NS_FAILURE)
    {
      nsLog::Error("Could not open for reading: '{0}'", sFile);
      return NS_FAILURE;
    }

    nsDynamicArray<nsUInt8> FileContent;

    nsUInt8 Temp[4024];
    nsUInt64 uiRead = File.ReadBytes(Temp, NS_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(nsArrayPtr<nsUInt8>(Temp, (nsUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, NS_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!nsUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      nsLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                   "an editor that does not save the file in Utf8 encoding.",
        sFile);
      return NS_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    return NS_SUCCESS;
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const nsUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // get a directory iterator for the search directory
    nsFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), nsFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      nsStringBuilder currentFile, sExt;

      // while there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // build the absolute path to the current file
        currentFile = it.GetCurrentPath();
        currentFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = currentFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h"))
        {
          nsLog::Info("Checking: {}", currentFile);

          NS_LOG_BLOCK("Header", &currentFile.GetData()[uiSearchDirLength]);
          CheckHeaderFile(currentFile);
          m_pStackAllocator->Reset();
        }
      }
    }
    else
      nsLog::Error("Could not search the directory '{0}'", m_sSearchDir);
  }

  void CheckInclude(const nsStringBuilder& sCurrentFile, const nsStringBuilder& sIncludePath, nsUInt32 uiLine)
  {
    nsStringBuilder absIncludePath(m_pStackAllocator.Borrow());
    bool includeOutside = true;
    if (sIncludePath.IsAbsolutePath())
    {
      for (auto& includeDir : m_IncludeDirectories)
      {
        if (sIncludePath.StartsWith(includeDir))
        {
          includeOutside = false;
          break;
        }
      }
    }
    else
    {
      bool includeFound = false;
      if (sIncludePath.StartsWith("ThirdParty"))
      {
        includeOutside = true;
      }
      else
      {
        for (auto& includeDir : m_IncludeDirectories)
        {
          absIncludePath = includeDir;
          absIncludePath.AppendPath(sIncludePath);
          if (nsOSFile::ExistsFile(absIncludePath))
          {
            includeOutside = false;
            break;
          }
        }
      }
    }

    if (includeOutside)
    {
      nsStringBuilder includeFileLower = sIncludePath.GetFileNameAndExtension();
      includeFileLower.ToLower();
      nsStringBuilder currentFileLower = sCurrentFile.GetFileNameAndExtension();
      currentFileLower.ToLower();

      // ignore includes ending in "_Platform.h", they redirect to platform specific NS headers
      const bool ignore = m_IgnoreTarget.m_byName.Contains(includeFileLower) || m_IgnoreSource.m_byName.Contains(currentFileLower) || includeFileLower.EndsWith_NoCase("_Platform.h");

      if (!ignore)
      {
        nsLog::Error("Including '{0}' in {1}:{2} leaks underlying implementation details. Including system or thirdparty headers in public ns header "
                     "files is not allowed. Please use an interface, factory or pimpl pattern to hide the implementation and avoid the include. See "
                     "the Documentation Chapter 'General->Header Files' for details.",
          sIncludePath.GetView(), sCurrentFile.GetView(), uiLine);
      }
    }
  }

  void CheckHeaderFile(const nsStringBuilder& sCurrentFile)
  {
    nsStringBuilder fileContents(m_pStackAllocator.Borrow());
    ReadEntireFile(sCurrentFile.GetData(), fileContents).IgnoreResult();

    auto fileDir = sCurrentFile.GetFileDirectory();

    nsStringBuilder internalMacroToken(m_pStackAllocator.Borrow());
    internalMacroToken.Append("NS_", m_sProjectName, "_INTERNAL_HEADER");
    auto internalMacroTokenView = internalMacroToken.GetView();

    nsTokenizer tokenizer(m_pStackAllocator.Borrow());
    auto dataView = fileContents.GetView();
    tokenizer.Tokenize(nsArrayPtr<const nsUInt8>(reinterpret_cast<const nsUInt8*>(dataView.GetStartPointer()), dataView.GetElementCount()), nsLog::GetThreadLocalLogSystem());

    nsStringView hash("#");
    nsStringView include("include");
    nsStringView openAngleBracket("<");
    nsStringView closeAngleBracket(">");

    bool isInternalHeader = false;
    auto tokens = tokenizer.GetTokens();
    const auto numTokens = tokens.GetCount();
    for (nsUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == nsTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == nsTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == nsTokenType::Whitespace);

        if (curToken.m_iType == nsTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == nsTokenType::Whitespace);

          if (curToken.m_iType == nsTokenType::String1)
          {
            // #include "bla"
            nsStringBuilder absIncludePath(m_pStackAllocator.Borrow());
            nsStringBuilder relativePath(m_pStackAllocator.Borrow());
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!nsOSFile::ExistsFile(absIncludePath))
            {
              nsLog::Error("The file '{0}' does not exist. Includes relative to the global include directories should use the #include "
                           "<path/to/file.h> syntax.",
                absIncludePath);
            }
            else if (!isInternalHeader)
            {
              CheckInclude(sCurrentFile, absIncludePath, includeToken.m_uiLine);
            }
          }
          else if (curToken.m_iType == nsTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == nsTokenType::Newline)
              {
                nsLog::Error("Non-terminated '<' in #include {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != nsTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != nsTokenType::Newline);
            }
            else if (!isInternalHeader)
            {
              nsStringBuilder includePath(m_pStackAllocator.Borrow());
              includePath = nsStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              CheckInclude(sCurrentFile, includePath, startToken.m_uiLine);
            }
          }
          else
          {
            // error
            nsLog::Error("Can not parse #include statement in {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != nsTokenType::Newline && curToken.m_iType != nsTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        if (curToken.m_iType == nsTokenType::Identifier && curToken.m_DataView == internalMacroTokenView)
        {
          isInternalHeader = true;
        }
        else
        {
          while (curToken.m_iType != nsTokenType::Newline && curToken.m_iType != nsTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
    }
  }

  virtual void Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
    {
      RequestApplicationQuit();
      return;
    }

    IterateOverFiles();
    RequestApplicationQuit();
  }
};

NS_APPLICATION_ENTRY_POINT(nsHeaderCheckApp);
