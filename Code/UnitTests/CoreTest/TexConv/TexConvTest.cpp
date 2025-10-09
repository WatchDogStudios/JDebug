#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if NS_ENABLED(NS_SUPPORTS_PROCESSES) && (NS_ENABLED(NS_PLATFORM_WINDOWS) || NS_ENABLED(NS_PLATFORM_LINUX)) && defined(BUILDSYSTEM_TEXCONV_PRESENT)

class nsTexConvTest : public nsTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TexConvTool"; }

  virtual nsResult GetImage(nsImage& ref_img, const nsSubTestEntry& subTest, nsUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_pState->m_image));
    return NS_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
    TGA,
  };

  virtual void SetupSubTests() override;

  virtual nsTestAppRun RunSubTest(nsInt32 iIdentifier, nsUInt32 uiInvocationCount) override;

  virtual nsResult InitializeTest() override
  {
    nsStartup::StartupCoreSystems();

    m_pState = NS_DEFAULT_NEW(State);

    const nsStringBuilder sReadDir(">sdk/", nsTestFramework::GetInstance()->GetRelTestDataPath());

    if (nsFileSystem::AddDataDirectory(sReadDir.GetData(), "TexConvTest", "testdata").Failed())
    {
      return NS_FAILURE;
    }

    nsFileSystem::AddDataDirectory(">nstest/", "TexConvDataDir", "imgout", nsDataDirUsage::AllowWrites).IgnoreResult();

    return NS_SUCCESS;
  }

  virtual nsResult DeInitializeTest() override
  {
    m_pState.Clear();

    nsFileSystem::RemoveDataDirectoryGroup("TexConvTest");
    nsFileSystem::RemoveDataDirectoryGroup("TexConvDataDir");

    nsStartup::ShutdownCoreSystems();

    return NS_SUCCESS;
  }

  void RunTexConv(nsProcessOptions& options, const char* szOutName)
  {
#  if NS_ENABLED(NS_PLATFORM_WINDOWS)
    const char* szTexConvExecutableName = "nsTexConv.exe";
#  else
    const char* szTexConvExecutableName = "nsTexConv";
#  endif
    nsStringBuilder sTexConvExe = nsOSFile::GetApplicationDirectory();
    sTexConvExe.AppendPath(szTexConvExecutableName);
    sTexConvExe.MakeCleanPath();

    if (!NS_TEST_BOOL_MSG(nsOSFile::ExistsFile(sTexConvExe), "%s does not exist", szTexConvExecutableName))
      return;

    options.m_sProcess = sTexConvExe;

    nsStringBuilder sOut = nsTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (!NS_TEST_BOOL(m_pState->m_TexConvGroup.Launch(options).Succeeded()))
      return;

    if (!NS_TEST_BOOL_MSG(m_pState->m_TexConvGroup.WaitToFinish(nsTime::MakeFromMinutes(1.0)).Succeeded(), "TexConv did not finish in time."))
      return;

    NS_TEST_INT_MSG(m_pState->m_TexConvGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TexConv failed to process the image");

    if (!NS_TEST_BOOL_MSG(m_pState->m_image.LoadFrom(sOut).Succeeded(), "Failed to load converted image"))
      return;

    nsByteBlobPtr rawImgData = m_pState->m_image.GetByteBlobPtr();
    nsUInt64 rawDataHash = nsHashingUtils::xxHash64(rawImgData.GetPtr(), rawImgData.GetCount(), 1234);
    // The [test] tag tells the UnitTest to actually output this:
    nsLog::Info("[test]Converted file '{0}' has raw data hash: 0x{1}", szOutName, nsArgU(rawDataHash, 16, true, 16, false));
  }

  struct State
  {
    nsProcessGroup m_TexConvGroup;
    nsImage m_image;
  };

  nsUniquePtr<State> m_pState;
};

void nsTexConvTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
  AddSubTest("TGA loading", SubTest::TGA);
}

nsTestAppRun nsTexConvTest::RunSubTest(nsInt32 iIdentifier, nsUInt32 uiInvocationCount)
{
  nsStringBuilder sImageData;
  nsFileSystem::ResolvePath(":testdata/TexConv", &sImageData, nullptr).IgnoreResult();

  const nsStringBuilder sPathNS(sImageData, "/NS.png");
  const nsStringBuilder sPathE(sImageData, "/E.png");
  const nsStringBuilder sPathZ(sImageData, "/Z.png");
  const nsStringBuilder sPathShape(sImageData, "/Shape.png");
  const nsStringBuilder sPathTGAv(sImageData, "/NS_flipped_v.tga");
  const nsStringBuilder sPathTGAh(sImageData, "/NS_flipped_h.tga");
  const nsStringBuilder sPathTGAvhCompressed(sImageData, "/NS_flipped_vh.tga");

  if (iIdentifier == SubTest::RgbaToRgbPNG)
  {
    nsProcessOptions opt;
    opt.AddArgument("-rgb");
    opt.AddArgument("in0");

    opt.AddArgument("-in0");
    opt.AddArgument(sPathNS);

    RunTexConv(opt, "RgbaToRgbPNG.png");

    NS_TEST_IMAGE(0, 10);
  }

  if (iIdentifier == SubTest::Combine4)
  {
    nsProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathNS);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in1.r");

    opt.AddArgument("-g");
    opt.AddArgument("in0.r");

    opt.AddArgument("-b");
    opt.AddArgument("in2.r");

    opt.AddArgument("-a");
    opt.AddArgument("in3.r");

    opt.AddArgument("-type");
    opt.AddArgument("2D");

    opt.AddArgument("-compression");
    opt.AddArgument("medium");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("linear");

    opt.AddArgument("-usage");
    opt.AddArgument("color");

    RunTexConv(opt, "Combine4.dds");

    // Threshold needs to be higher here since we might fall back to software dxt compression
    // which results in slightly different results than GPU dxt compression.
    NS_TEST_IMAGE(1, 100);
  }

  if (iIdentifier == SubTest::LinearUsage)
  {
    nsProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathNS);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in3");

    opt.AddArgument("-g");
    opt.AddArgument("in0");

    opt.AddArgument("-b");
    opt.AddArgument("in2");

    opt.AddArgument("-compression");
    opt.AddArgument("high");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("kaiser");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-downscale");
    opt.AddArgument("1");

    RunTexConv(opt, "Linear.dds");

    NS_TEST_IMAGE(2, 10);
  }

  if (iIdentifier == SubTest::ExtractChannel)
  {
    nsProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathNS);

    opt.AddArgument("-r");
    opt.AddArgument("in0.r");

    opt.AddArgument("-compression");
    opt.AddArgument("none");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("none");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-maxRes");
    opt.AddArgument("64");

    RunTexConv(opt, "ExtractChannel.dds");

    NS_TEST_IMAGE(3, 10);
  }

  if (iIdentifier == SubTest::TGA)
  {
    {
      nsProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAv);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "NS_flipped_v.dds");

      NS_TEST_IMAGE(3, 10);
    }

    {
      nsProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAh);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "NS_flipped_h.dds");

      NS_TEST_IMAGE(4, 10);
    }

    {
      nsProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAvhCompressed);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "NS_flipped_vh.dds");

      NS_TEST_IMAGE(5, 10);
    }
  }

  return nsTestAppRun::Quit;
}



static nsTexConvTest s_nsTexConvTest;

#endif
