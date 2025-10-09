#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ShaderCompiler/ShaderCompiler.h>

nsCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .nsShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
  "");

nsCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Absolute path to the folder of the project, for which shaders should be compiled.",
  "");

nsCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform DX11_SM50\n\
  -platform VULKAN\n\
  -platform ALL",
  "DX11_SM50");

nsCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

nsCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE MSAA=TRUE\n\
\n\
If a permutation variable is not set to a fixed value, all shader permutations for that variable will generated and compiled.\n\
",
  "");

nsShaderCompilerApplication::nsShaderCompilerApplication()
  : nsGameApplication("nsShaderCompiler", nullptr)
{
}

nsResult nsShaderCompilerApplication::BeforeCoreSystemsStartup()
{
  {
    nsStringBuilder cmdHelp;
    if (nsCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, nsCommandLineOption::LogAvailableModes::IfHelpRequested, "_ShaderCompiler"))
    {
      nsLog::Print(cmdHelp);
      return NS_FAILURE;
    }
  }

  nsStartup::AddApplicationTag("tool");
  nsStartup::AddApplicationTag("shadercompiler");

  // only print important messages
  nsLog::SetDefaultLogLevel(nsLogMsgType::InfoMsg);

  NS_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  auto cmd = nsCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = opt_Shader.GetOptionValue(nsCommandLineOption::LogMode::Always);
  m_sAppProjectPath = opt_Project.GetOptionValue(nsCommandLineOption::LogMode::Always);
  m_sPlatforms = opt_Platform.GetOptionValue(nsCommandLineOption::LogMode::Always);
  opt_IgnoreErrors.GetOptionValue(nsCommandLineOption::LogMode::Always);

  const nsUInt32 pvs = cmd->GetStringOptionArguments("-perm");

  for (nsUInt32 pv = 0; pv < pvs; ++pv)
  {
    nsStringBuilder var = cmd->GetStringOption("-perm", pv);

    const char* szEqual = var.FindSubString("=");

    if (szEqual == nullptr)
    {
      nsLog::Error("Permutation Variable declaration contains no equal sign: '{0}'", var);
      continue;
    }

    nsStringBuilder val = szEqual + 1;
    var.SetSubString_FromTo(var.GetData(), szEqual);

    val.Trim(" \t");
    var.Trim(" \t");

    nsLog::Dev("Fixed permutation variable: {0} = {1}", var, val);
    m_FixedPermVars[var].PushBack(val);
  }

  return NS_SUCCESS;
}


void nsShaderCompilerApplication::AfterCoreSystemsStartup()
{
  nsSystemInformation info = nsSystemInformation::Get();
  const nsInt32 iCpuCores = info.GetCPUCoreCount();
  nsTaskSystem::SetWorkerThreadCount(iCpuCores);

  ExecuteInitFunctions();

  nsStartup::StartupHighLevelSystems();
}

nsResult nsShaderCompilerApplication::CompileShader(nsStringView sShaderFile)
{
  NS_PROFILE_SCOPE("nsShaderCompilerApplication::CompileShader");
  NS_LOG_BLOCK("Compiling Shader", sShaderFile);

  if (ExtractPermutationVarValues(sShaderFile).Failed())
    return NS_FAILURE;


  const nsUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  nsLog::Info("Shader has {0} permutations", uiMaxPerms);

  bool bContinue = true;

  nsTaskSystem::ParallelForIndexed(0, uiMaxPerms, [&](nsUInt32 idx, nsUInt32 num)
    {
      if (!bContinue)
        return;

      nsHybridArray<nsPermutationVar, 16> PermVars;

      nsTokenizedFileCache fileCache;
      for (nsUInt32 perm = idx; perm < num; ++perm)
      {
        NS_PROFILE_SCOPE("CompilePermutation");
        NS_LOG_BLOCK("Compiling Permutation");

        m_PermutationGenerator.GetPermutation(perm, PermVars);
        nsShaderCompiler sc;
        if (sc.CompileShaderPermutationForPlatforms(sShaderFile, PermVars, nsLog::GetThreadLocalLogSystem(), m_sPlatforms, &fileCache).Failed())
        {
          bContinue = false;
          return;
        }
      }
      //
    });

  if (!bContinue)
  {
    nsLog::Error("Failed to compile shader '{0}'", sShaderFile);
    return NS_FAILURE;
  }

  nsLog::Success("Compiled Shader '{0}'", sShaderFile);
  return NS_SUCCESS;
}

nsResult nsShaderCompilerApplication::ExtractPermutationVarValues(nsStringView sShaderFile)
{
  NS_PROFILE_SCOPE("nsShaderCompilerApplication::ExtractPermutationVarValues");

  m_PermutationGenerator.Clear();

  nsFileReader shaderFile;
  if (shaderFile.Open(sShaderFile).Failed())
  {
    nsLog::Error("Could not open file '{0}'", sShaderFile);
    return NS_FAILURE;
  }

  nsString sContent;
  sContent.ReadAll(shaderFile);

  nsShaderHelper::nsTextSectionizer Sections;
  nsShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  nsHybridArray<nsHashedString, 16> permVars;
  nsHybridArray<nsPermutationVar, 16> fixedPermVars;
  nsUInt32 uiFirstLine = 0;
  nsStringView sPermutations = Sections.GetSectionContent(nsShaderHelper::nsShaderSections::PERMUTATIONS, uiFirstLine);
  nsShaderParser::ParsePermutationSection(sPermutations, permVars, fixedPermVars);

  {
    NS_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      nsLog::Dev(s.GetData());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permVars)
    {
      nsHybridArray<nsHashedString, 16> values;
      nsShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermVars)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      nsHashedString hsname, hsvalue;
      hsname.Assign(it.Key().GetData());
      m_PermutationGenerator.RemovePermutations(hsname);

      for (const auto& val : it.Value())
      {
        hsvalue.Assign(val.GetData());

        m_PermutationGenerator.AddPermutation(hsname, hsvalue);
      }
    }
  }

  return NS_SUCCESS;
}

void nsShaderCompilerApplication::PrintConfig()
{
  NS_LOG_BLOCK("ShaderCompiler Config");

  nsLog::Info("Project: '{0}'", m_sAppProjectPath);
  nsLog::Info("Shader: '{0}'", m_sShaderFiles);
  nsLog::Info("Platform: '{0}'", m_sPlatforms);
}

void nsShaderCompilerApplication::Run()
{
  PrintConfig();

  NS_LOG_BLOCK("Compile All Shaders");

  nsDynamicArray<nsString> shadersToCompile;

  nsStringBuilder files = m_sShaderFiles;

  nsDynamicArray<nsStringView> allFiles;
  // If not shader files are provided, compile all shaders of the project, i.e. all data directories.
  if (m_sShaderFiles.IsEmpty())
  {
    nsStringBuilder sPath, sPath2;
    for (nsUInt32 dirIdx = 0; dirIdx < nsFileSystem::GetNumDataDirectories(); ++dirIdx)
    {
      sPath = nsFileSystem::GetDataDirectory(dirIdx)->GetDataDirectoryPath();

      if (sPath.IsEmpty())
        continue;

      if (nsFileSystem::ResolveSpecialDirectory(sPath, sPath2).Failed())
        continue;

      files.AppendWithSeparator(";", sPath2);
    }
  }

  files.Split(false, allFiles, ";");

  nsUInt32 uiErrors = 0;
  for (const nsStringView& entry : allFiles)
  {
    nsStringBuilder fileOrFolder;
    // Relative paths are always relative to the project
    if (nsPathUtils::IsRelativePath(entry))
    {
      fileOrFolder = m_sAppProjectPath;
      fileOrFolder.AppendPath(entry);
    }
    else
    {
      fileOrFolder = entry;
    }

    nsFileStats stats;
    if (nsOSFile::GetFileStats(fileOrFolder, stats).Failed())
    {
      nsLog::Error("Couldn't find path '{0}'", fileOrFolder);
      ++uiErrors;
      continue;
    }

    nsStringBuilder relPath, absPath;
    if (stats.m_bIsDirectory)
    {
      nsFileSystemIterator fsIt;
      nsStringBuilder fullPath;
      for (fsIt.StartSearch(fileOrFolder, nsFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
      {
        if (nsPathUtils::HasExtension(fsIt.GetStats().m_sName, "nsShader"))
        {
          fsIt.GetStats().GetFullPath(fullPath);
          if (nsFileSystem::ResolvePath(fullPath, &absPath, &relPath).Succeeded())
          {
            shadersToCompile.PushBack(relPath);
          }
          else
          {
            nsLog::Error("Couldn't resolve path '{0}'", fullPath);
            ++uiErrors;
          }
        }
      }
    }
    else if (nsFileSystem::ResolvePath(fileOrFolder, &absPath, &relPath).Succeeded())
    {
      if (absPath.HasExtension("nsShader"))
      {
        shadersToCompile.PushBack(relPath);
      }
      else
      {
        nsLog::Error("File '{0}' is not a shader", absPath);
        ++uiErrors;
      }
    }
    else
    {
      nsLog::Error("Couldn't resolve path '{0}'", fileOrFolder);
    }
  }

  for (const auto& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      ++uiErrors;
      if (!opt_IgnoreErrors.GetOptionValue(nsCommandLineOption::LogMode::Never))
      {
        SetReturnCode(uiErrors);
        RequestApplicationQuit();
        return;
      }
    }
  }
  SetReturnCode(uiErrors);
  RequestApplicationQuit();
}

NS_APPLICATION_ENTRY_POINT(nsShaderCompilerApplication);
