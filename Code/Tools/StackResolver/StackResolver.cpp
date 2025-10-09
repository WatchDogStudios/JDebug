#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/StackTracer.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <DbgHelp.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

struct Module
{
  nsString m_sFilePath;
  nsUInt64 m_uiBaseAddress;
  nsUInt32 m_uiSize;
};

struct Stackframe
{
  nsUInt32 m_uiModuleIndex = 0xFFFFFFFF;
  nsUInt32 m_uiLineNumber = 0;
  nsString m_sFilename;
  nsString m_sSymbol;
};

nsCommandLineOptionString opt_ModuleList("_app", "-ModuleList", "List of modules as a string in this format:\n\n\
File1Path?File1BaseAddressHEX?File1Size|File2Path?File2BaseAddressHEX?File2Size|...\n\n\
For example:\n\
  $[A]/app.exe?7FF7E5540000?106496|$[S]/System32/KERNELBASE.dll?7FFE2B780000?2920448\n\n\
  $[A] represents the application directory and will be adjusted as necessary.\n\
  $[S] represents the system root directory and will be adjusted as necessary.",
  "");
nsCommandLineOptionString opt_Callstack("_app", "-Callstack", "Callstack in this format:\n\n7FFE2DD6CE74|7FFE2B7AAA86|7FFE034C22D1", "");

nsCommandLineOptionEnum opt_OutputFormat("_app", "-Format", "How to output the resolved callstack.", "Text=0|JSON=1", 0);

nsCommandLineOptionPath opt_OutputFile("_app", "-File", "The target file where to write the output to.\nIf left empty, the output is printed to the console.", "");

class nsStackResolver : public nsApplication
{
public:
  using SUPER = nsApplication;

  nsStackResolver()
    : nsApplication("nsStackResolver")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Run() override;

  nsResult LoadModules();
  nsResult ParseModules();
  nsResult ParseCallstack();

  void ResolveStackFrames();
  void FormatAsText(nsStringBuilder& ref_sOutput);
  void FormatAsJSON(nsStringBuilder& ref_sOutput);

  HANDLE m_hProcess;
  nsDynamicArray<Module> m_Modules;
  nsDynamicArray<nsUInt64> m_Callstack;
  nsDynamicArray<Stackframe> m_Stackframes;
  nsStringBuilder m_SystemRootDir;
  nsStringBuilder m_ApplicationDir;
};

void nsStackResolver::AfterCoreSystemsStartup()
{
  nsGlobalLog::AddLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::AddLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);

  m_Modules.Reserve(128);
  m_Callstack.Reserve(128);
  m_Stackframes.Reserve(128);
}

void nsStackResolver::BeforeCoreSystemsShutdown()
{
  nsGlobalLog::RemoveLogWriter(nsLogWriter::Console::LogMessageHandler);
  nsGlobalLog::RemoveLogWriter(nsLogWriter::VisualStudio::LogMessageHandler);
}

nsResult nsStackResolver::ParseModules()
{
  const nsStringBuilder sModules = opt_ModuleList.GetOptionValue(nsCommandLineOption::LogMode::Never);

  nsDynamicArray<nsStringView> parts;
  sModules.Split(false, parts, "|");

  for (nsStringView sModView : parts)
  {
    nsStringBuilder sMod = sModView;
    nsDynamicArray<nsStringView> parts2;
    sMod.Split(false, parts2, "?");

    nsUInt64 base;
    if (nsConversionUtils::ConvertHexStringToUInt64(parts2[1], base).Failed())
    {
      nsLog::Error("Failed to convert HEX string '{}' to UINT64", parts2[1]);
      return NS_FAILURE;
    }

    nsStringBuilder sSize = parts2[2];
    nsUInt32 size;
    if (nsConversionUtils::StringToUInt(sSize, size).Failed())
    {
      nsLog::Error("Failed to convert string '{}' to UINT32", sSize);
      return NS_FAILURE;
    }

    nsStringBuilder sModuleName = parts2[0];
    sModuleName.ReplaceFirst_NoCase("$[S]", m_SystemRootDir);
    sModuleName.ReplaceFirst_NoCase("$[A]", m_ApplicationDir);
    sModuleName.MakeCleanPath();

    auto& mod = m_Modules.ExpandAndGetRef();
    mod.m_sFilePath = sModuleName;
    mod.m_uiBaseAddress = base;
    mod.m_uiSize = size;
  }

  return NS_SUCCESS;
}

nsResult nsStackResolver::ParseCallstack()
{
  nsStringBuilder sCallstack = opt_Callstack.GetOptionValue(nsCommandLineOption::LogMode::Never);

  nsDynamicArray<nsStringView> parts;
  sCallstack.Split(false, parts, "|");
  for (nsStringView sModView : parts)
  {
    nsUInt64 base;
    if (nsConversionUtils::ConvertHexStringToUInt64(sModView, base).Failed())
    {
      nsLog::Error("Failed to convert HEX string '{}' to UINT64", sModView);
      return NS_FAILURE;
    }

    m_Callstack.PushBack(base);
  }

  return NS_SUCCESS;
}

nsResult nsStackResolver::LoadModules()
{
  if (SymInitialize(m_hProcess, nullptr, FALSE) != TRUE) // TODO specify PDB search path as second parameter?
  {
    nsLog::Error("SymInitialize failed");
    return NS_FAILURE;
  }

  for (const auto& curModule : m_Modules)
  {
    if (SymLoadModuleExW(m_hProcess, nullptr, nsStringWChar(curModule.m_sFilePath), nullptr, curModule.m_uiBaseAddress, curModule.m_uiSize, nullptr, 0) == 0)
    {
      nsLog::Warning("Couldn't load module '{}'", curModule.m_sFilePath);
    }
    else
    {
      nsLog::Success("Loaded module '{}'", curModule.m_sFilePath);
    }
  }

  return NS_SUCCESS;
}

void nsStackResolver::ResolveStackFrames()
{
  nsStringBuilder tmp;

  char buffer[1024];
  for (nsUInt32 i = 0; i < m_Callstack.GetCount(); i++)
  {
    DWORD64 symbolAddress = m_Callstack[i];

    _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
    nsMemoryUtils::ZeroFill(&symbolInfo, 1);
    symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
    symbolInfo.MaxNameLen = (NS_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

    DWORD64 displacement = 0;
    BOOL result = SymFromAddrW(m_hProcess, symbolAddress, &displacement, &symbolInfo);
    if (!result)
    {
      wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
    }

    IMAGEHLP_LINEW64 lineInfo;
    DWORD displacement2 = static_cast<DWORD>(displacement);
    nsMemoryUtils::ZeroFill(&lineInfo, 1);
    lineInfo.SizeOfStruct = sizeof(lineInfo);
    SymGetLineFromAddrW64(m_hProcess, symbolAddress, &displacement2, &lineInfo);

    auto& frame = m_Stackframes.ExpandAndGetRef();

    for (nsUInt32 modIndex = 0; modIndex < m_Modules.GetCount(); modIndex++)
    {
      if (m_Modules[modIndex].m_uiBaseAddress == (nsUInt64)symbolInfo.ModBase)
      {
        frame.m_uiModuleIndex = modIndex;
        break;
      }
    }

    frame.m_uiLineNumber = (nsUInt32)lineInfo.LineNumber;
    frame.m_sSymbol = nsStringUtf8(symbolInfo.Name).GetView();

    tmp = nsStringUtf8(lineInfo.FileName).GetView();
    tmp.MakeCleanPath();
    frame.m_sFilename = tmp;
  }
}

void nsStackResolver::FormatAsText(nsStringBuilder& ref_sOutput)
{
  nsLog::Info("Formatting callstack as text.");

  for (const auto& frame : m_Stackframes)
  {
    nsStringView sModuleName = "<unknown module>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    nsStringView sFileName = "<unknown file>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    nsStringView sSymbol = "<unknown symbol>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    ref_sOutput.AppendFormat("[][{}] {}({}): '{}'\n", sModuleName, sFileName, frame.m_uiLineNumber, sSymbol);
  }
}

void nsStackResolver::FormatAsJSON(nsStringBuilder& ref_sOutput)
{
  nsLog::Info("Formatting callstack as JSON.");

  nsContiguousMemoryStreamStorage storage;
  nsMemoryStreamWriter writer(&storage);

  nsStandardJSONWriter json;
  json.SetOutputStream(&writer);
  json.SetWhitespaceMode(nsJSONWriter::WhitespaceMode::LessIndentation);

  json.BeginObject();
  json.BeginArray("Stackframes");

  for (const auto& frame : m_Stackframes)
  {
    nsStringView sModuleName = "<unknown>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    nsStringView sFileName = "<unknown>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    nsStringView sSymbol = "<unknown>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    json.BeginObject();
    json.AddVariableString("Module", sModuleName);
    json.AddVariableString("File", sFileName);
    json.AddVariableUInt32("Line", frame.m_uiLineNumber);
    json.AddVariableString("Symbol", sSymbol);
    json.EndObject();
  }

  json.EndArray();
  json.EndObject();

  nsStringView text((const char*)storage.GetData(), storage.GetStorageSize32());

  ref_sOutput.Append(text);
}

void nsStackResolver::Run()
{
  if (nsCommandLineOption::LogAvailableOptions(nsCommandLineOption::LogAvailableModes::IfHelpRequested, "_app"))
  {
    RequestApplicationQuit();
    return;
  }

  nsString sMissingOpt;
  if (nsCommandLineOption::RequireOptions("-ModuleList;-Callstack", &sMissingOpt).Failed())
  {
    nsLog::Error("Command line option '{}' was not specified.", sMissingOpt);

    nsCommandLineOption::LogAvailableOptions(nsCommandLineOption::LogAvailableModes::Always, "_app");
    RequestApplicationQuit();
    return;
  }

  m_hProcess = GetCurrentProcess();

  m_ApplicationDir = nsOSFile::GetApplicationDirectory();
  m_ApplicationDir.MakeCleanPath();
  m_ApplicationDir.Trim("", "/");

  m_SystemRootDir = nsEnvironmentVariableUtils::GetValueString("SystemRoot");
  m_SystemRootDir.MakeCleanPath();
  m_SystemRootDir.Trim("", "/");

  if (ParseModules().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  if (ParseCallstack().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  if (LoadModules().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  ResolveStackFrames();

  nsStringBuilder output;

  if (opt_OutputFormat.GetOptionValue(nsCommandLineOption::LogMode::Never) == 0)
  {
    FormatAsText(output);
  }
  else if (opt_OutputFormat.GetOptionValue(nsCommandLineOption::LogMode::Never) == 1)
  {
    FormatAsJSON(output);
  }

  if (opt_OutputFile.IsOptionSpecified())
  {
    nsLog::Info("Writing output to '{}'.", opt_OutputFile.GetOptionValue(nsCommandLineOption::LogMode::Never));

    nsOSFile file;
    if (file.Open(opt_OutputFile.GetOptionValue(nsCommandLineOption::LogMode::Never), nsFileOpenMode::Write).Failed())
    {
      nsLog::Error("Could not open file for writing: '{}'", opt_OutputFile.GetOptionValue(nsCommandLineOption::LogMode::Never));
      RequestApplicationQuit();
      return;
    }

    file.Write(output.GetData(), output.GetElementCount()).IgnoreResult();
  }
  else
  {
    nsLog::Info("Writing output to console.");

    NS_LOG_BLOCK("Resolved callstack");

    nsDynamicArray<nsStringView> lines;
    output.Split(true, lines, "\n");

    for (auto l : lines)
    {
      nsLog::Info("{}", l);
    }
  }

  RequestApplicationQuit();
}

NS_APPLICATION_ENTRY_POINT(nsStackResolver);
