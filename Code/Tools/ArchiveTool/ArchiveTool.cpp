#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>

/* ArchiveTool command line options:

-out <path>
    Path to a file or folder.

    -out specifies the target to pack or unpack things to.
    For packing mode it has to be a file. The file will be overwritten, if it already exists.
    For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.

    If no -out is specified, it is determined to be where the input file is located.

-unpack <paths>
    One or multiple paths to nsArchive files that shall be extracted.

    Example:
      -unpack "path/to/file.nsArchive" "another/file.nsArchive"

-pack <paths>
    One or multiple paths to folders that shall be packed.

    Example:
      -pack "path/to/folder" "path/to/another/folder"

Description:
    -pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
    or to unpack multiple archives at the same time.

    If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
    If all inputs are folders, the mode is 'pack'.
    If all inputs are files, the mode is 'unpack'.

Examples:
    nsArchiveTool.exe "C:/Stuff"
      Packs all data in "C:/Stuff" into "C:/Stuff.nsArchive"

    nsArchiveTool.exe "C:/Stuff" -out "C:/MyStuff.nsArchive"
      Packs all data in "C:/Stuff" into "C:/MyStuff.nsArchive"

    nsArchiveTool.exe "C:/Stuff.nsArchive"
      Unpacks all data from the archive into "C:/Stuff"

    nsArchiveTool.exe "C:/Stuff.nsArchive" -out "C:/MyStuff"
      Unpacks all data from the archive into "C:/MyStuff"
*/

nsCommandLineOptionPath opt_Out("_ArchiveTool", "-out", "\
Path to a file or folder.\n\
\n\
-out specifies the target to pack or unpack things to.\n\
For packing mode it has to be a file. The file will be overwritten, if it already exists.\n\
For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.\n\
\n\
If no -out is specified, it is determined to be where the input file is located.\n\
",
  "");

nsCommandLineOptionDoc opt_Unpack("_ArchiveTool", "-unpack", "<paths>", "\
One or multiple paths to nsArchive files that shall be extracted.\n\
\n\
Example:\n\
  -unpack \"path/to/file.nsArchive\" \"another/file.nsArchive\"\n\
",
  "");

nsCommandLineOptionDoc opt_Pack("_ArchiveTool", "-pack", "<paths>", "\
One or multiple paths to folders that shall be packed.\n\
\n\
Example:\n\
  -pack \"path/to/folder\" \"path/to/another/folder\"\n\
",
  "");

nsCommandLineOptionDoc opt_Desc("_ArchiveTool", "Description:", "", "\
-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)\n\
or to unpack multiple archives at the same time.\n\
\n\
If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.\n\
If all inputs are folders, the mode is 'pack'.\n\
If all inputs are files, the mode is 'unpack'.\n\
",
  "");

nsCommandLineOptionDoc opt_Examples("_ArchiveTool", "Examples:", "", "\
nsArchiveTool.exe \"C:/Stuff\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/Stuff.nsArchive\"\n\
\n\
nsArchiveTool.exe \"C:/Stuff\" -out \"C:/MyStuff.nsArchive\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/MyStuff.nsArchive\"\n\
\n\
nsArchiveTool.exe \"C:/Stuff.nsArchive\"\n\
  Unpacks all data from the archive into \"C:/Stuff\"\n\
\n\
nsArchiveTool.exe \"C:/Stuff.nsArchive\" -out \"C:/MyStuff\"\n\
  Unpacks all data from the archive into \"C:/MyStuff\"\n\
",
  "");

class nsArchiveBuilderImpl : public nsArchiveBuilder
{
protected:
  virtual void WriteFileResultCallback(nsUInt32 uiCurEntry, nsUInt32 uiMaxEntries, nsStringView sSourceFile, nsUInt64 uiSourceSize, nsUInt64 uiStoredSize, nsTime duration) const override
  {
    const nsUInt64 uiPercentage = (uiSourceSize == 0) ? 100 : (uiStoredSize * 100 / uiSourceSize);
    nsLog::Info(" [{}%%] {} ({}%%) - {}", nsArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile, uiPercentage, duration);
  }
};

class nsArchiveReaderImpl : public nsArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(nsUInt32 uiCurEntry, nsUInt32 uiMaxEntries, nsStringView sSourceFile) const override
  {
    nsLog::Info(" [{}%%] {}", nsArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile);
    return true;
  }


  virtual bool ExtractFileProgressCallback(nsUInt64 bytesWritten, nsUInt64 bytesTotal) const override
  {
    // nsLog::Dev("   {}%%", nsArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class nsArchiveTool : public nsApplication
{
public:
  using SUPER = nsApplication;

  enum class ArchiveMode
  {
    Auto,
    Pack,
    Unpack,
  };

  ArchiveMode m_Mode = ArchiveMode::Auto;

  nsDynamicArray<nsString> m_sInputs;
  nsString m_sOutput;

  nsArchiveTool()
    : nsApplication("ArchiveTool")
  {
  }

  nsResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      nsLog::Error("No arguments given");
      return NS_FAILURE;
    }

    nsCommandLineUtils& cmd = *nsCommandLineUtils::GetGlobalInstance();

    m_sOutput = opt_Out.GetOptionValue(nsCommandLineOption::LogMode::Always);

    nsStringBuilder path;

    if (cmd.GetStringOptionArguments("-pack") > 0)
    {
      m_Mode = ArchiveMode::Pack;
      const nsUInt32 args = cmd.GetStringOptionArguments("-pack");

      if (args == 0)
      {
        nsLog::Error("-pack option expects at least one argument");
        return NS_FAILURE;
      }

      for (nsUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-pack", a));

        if (!nsOSFile::ExistsDirectory(m_sInputs.PeekBack()))
        {
          nsLog::Error("-pack input path is not a valid directory: '{}'", m_sInputs.PeekBack());
          return NS_FAILURE;
        }
      }
    }
    else if (cmd.GetStringOptionArguments("-unpack") > 0)
    {
      m_Mode = ArchiveMode::Unpack;
      const nsUInt32 args = cmd.GetStringOptionArguments("-unpack");

      if (args == 0)
      {
        nsLog::Error("-unpack option expects at least one argument");
        return NS_FAILURE;
      }

      for (nsUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-unpack", a));

        if (!nsOSFile::ExistsFile(m_sInputs.PeekBack()))
        {
          nsLog::Error("-unpack input file does not exist: '{}'", m_sInputs.PeekBack());
          return NS_FAILURE;
        }
      }
    }
    else
    {
      bool bInputsFolders = true;
      bool bInputsFiles = true;

      for (nsUInt32 a = 1; a < GetArgumentCount(); ++a)
      {
        const nsStringView sArg = GetArgument(a);

        if (sArg.IsEqual_NoCase("-out"))
          break;

        m_sInputs.PushBack(nsOSFile::MakePathAbsoluteWithCWD(sArg));

        if (!nsOSFile::ExistsDirectory(m_sInputs.PeekBack()))
          bInputsFolders = false;
        if (!nsOSFile::ExistsFile(m_sInputs.PeekBack()))
          bInputsFiles = false;
      }

      if (bInputsFolders && !bInputsFiles)
      {
        m_Mode = ArchiveMode::Pack;
      }
      else if (bInputsFiles && !bInputsFolders)
      {
        m_Mode = ArchiveMode::Unpack;
      }
      else
      {
        nsLog::Error("Inputs are ambiguous. Specify only folders for packing or only files for unpacking. Use -out as last argument to "
                     "specify a target.");
        return NS_FAILURE;
      }
    }

    nsLog::Info("Mode is: {}", m_Mode == ArchiveMode::Pack ? "pack" : "unpack");
    nsLog::Info("Inputs:");

    for (const auto& input : m_sInputs)
    {
      nsLog::Info("  '{}'", input);
    }

    nsLog::Info("Output: '{}'", m_sOutput);

    return NS_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    nsFileSystem::AddDataDirectory("", "App", ":", nsDataDirUsage::AllowWrites).IgnoreResult();

    nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
    nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  static nsArchiveBuilder::InclusionMode PackFileCallback(nsStringView sFile)
  {
    const nsStringView ext = nsPathUtils::GetFileExtension(sFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return nsArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return nsArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return nsArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("dds"))
      return nsArchiveBuilder::InclusionMode::Compress_zstd_fast;

    return nsArchiveBuilder::InclusionMode::Compress_zstd_average;
  }

  nsResult Pack()
  {
    nsArchiveBuilderImpl archive;

    for (const auto& folder : m_sInputs)
    {
      archive.AddFolder(folder, nsArchiveCompressionMode::Compressed_zstd, PackFileCallback);
    }

    if (m_sOutput.IsEmpty())
    {
      nsStringBuilder sArchive = m_sInputs[0];
      sArchive.Append(".nsArchive");

      m_sOutput = sArchive;
    }

    m_sOutput = nsOSFile::MakePathAbsoluteWithCWD(m_sOutput);

    nsLog::Info("Writing archive to '{}'", m_sOutput);
    if (archive.WriteArchive(m_sOutput).Failed())
    {
      nsLog::Error("Failed to write the nsArchive");

      return NS_FAILURE;
    }

    return NS_SUCCESS;
  }

  nsResult Unpack()
  {
    for (const auto& file : m_sInputs)
    {
      nsLog::Info("Extracting archive '{}'", file);

      // if the file has a custom archive file extension, just register it as 'allowed'
      // we assume that the user only gives us files that are nsArchives
      if (!nsArchiveUtils::IsAcceptedArchiveFileExtensions(nsPathUtils::GetFileExtension(file)))
      {
        nsArchiveUtils::GetAcceptedArchiveFileExtensions().PushBack(nsPathUtils::GetFileExtension(file));
      }

      nsArchiveReaderImpl reader;
      NS_SUCCEED_OR_RETURN(reader.OpenArchive(file));

      nsStringBuilder sOutput = m_sOutput;

      if (sOutput.IsEmpty())
      {
        sOutput = file;
        sOutput.RemoveFileExtension();
      }

      if (reader.ExtractAllFiles(sOutput).Failed())
      {
        nsLog::Error("File extraction failed.");
        return NS_FAILURE;
      }
    }

    return NS_SUCCESS;
  }

  virtual void Run() override
  {
    {
      nsStringBuilder cmdHelp;
      if (nsCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, nsCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
      {
        nsLog::Print(cmdHelp);
        RequestApplicationQuit();
        return;
      }
    }

    nsStopwatch sw;

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      RequestApplicationQuit();
      return;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        nsLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      nsLog::Success("Finished packing archive in {}", sw.GetRunningTotal());
      RequestApplicationQuit();
      return;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        nsLog::Error("Extracting files failed");
        SetReturnCode(3);
      }

      nsLog::Success("Finished extracting archive in {}", sw.GetRunningTotal());
      RequestApplicationQuit();
      return;
    }

    nsLog::Error("Unknown mode");
    RequestApplicationQuit();
  }
};

NS_APPLICATION_ENTRY_POINT(nsArchiveTool);
