#include <Core/CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(nsWindWorldModuleInterface, 1, nsRTTINoAllocator)
NS_END_DYNAMIC_REFLECTED_TYPE;

NS_BEGIN_STATIC_REFLECTED_ENUM(nsWindStrength, 1)
  NS_ENUM_CONSTANTS(nsWindStrength::Calm, nsWindStrength::LightBrense, nsWindStrength::GentleBrense, nsWindStrength::ModerateBrense, nsWindStrength::StrongBrense, nsWindStrength::Storm)
  NS_ENUM_CONSTANTS(nsWindStrength::WeakShockwave, nsWindStrength::MediumShockwave, nsWindStrength::StrongShockwave, nsWindStrength::ExtremeShockwave)
NS_END_STATIC_REFLECTED_ENUM;
// clang-format on

float nsWindStrength::GetInMetersPerSecond(Enum strength)
{
  // inspired by the Beaufort scale
  // https://en.wikipedia.org/wiki/Beaufort_scale

  switch (strength)
  {
    case Calm:
      return 0.5f;

    case LightBrense:
      return 2.0f;

    case GentleBrense:
      return 5.0f;

    case ModerateBrense:
      return 9.0f;

    case StrongBrense:
      return 14.0f;

    case Storm:
      return 20.0f;

    case WeakShockwave:
      return 40.0f;

    case MediumShockwave:
      return 70.0f;

    case StrongShockwave:
      return 100.0f;

    case ExtremeShockwave:
      return 150.0f;

      NS_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

nsWindWorldModuleInterface::nsWindWorldModuleInterface(nsWorld* pWorld)
  : nsWorldModule(pWorld)
{
}

nsSimdVec4f nsWindWorldModuleInterface::GetWindAtSimd(const nsSimdVec4f& vPosition) const
{
  return nsSimdConversion::ToVec3(GetWindAt(nsSimdConversion::ToVec3(vPosition)));
}

nsVec3 nsWindWorldModuleInterface::ComputeWindFlutter(const nsVec3& vWind, const nsVec3& vObjectDir, float fFlutterSpeed, nsUInt32 uiFlutterRandomOffset) const
{
  if (vWind.IsZero(0.001f))
    return nsVec3::MakeZero();

  nsVec3 windDir = vWind;
  const float fWindStrength = windDir.GetLengthAndNormalize();

  if (fWindStrength <= 0.01f)
    return nsVec3::MakeZero();

  nsVec3 mainDir = vObjectDir;
  mainDir.NormalizeIfNotZero(nsVec3::MakeAxisZ()).IgnoreResult();

  nsVec3 flutterDir = windDir.CrossRH(mainDir);
  flutterDir.NormalizeIfNotZero(nsVec3::MakeAxisZ()).IgnoreResult();

  const float fFlutterOffset = (uiFlutterRandomOffset & 1023u) / 256.0f;

  const float fFlutter = nsMath::Sin(nsAngle::MakeFromRadian(fFlutterOffset + fFlutterSpeed * fWindStrength * GetWorld()->GetClock().GetAccumulatedTime().AsFloatInSeconds())) * fWindStrength;

  return flutterDir * fFlutter;
}

NS_STATICLINK_FILE(Core, Core_Interfaces_WindWorldModule);
