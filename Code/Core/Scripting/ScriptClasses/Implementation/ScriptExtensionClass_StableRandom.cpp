#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_StableRandom.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_StableRandom, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(IntMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    NS_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOne, Inout, "Position", In, "Seed"),
    NS_SCRIPT_FUNCTION_PROPERTY(FloatMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    NS_SCRIPT_FUNCTION_PROPERTY(Vec3MinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("StableRandom"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
int nsScriptExtensionClass_StableRandom::IntMinMax(int& inout_iPosition, int iMinValue, int iMaxValue, nsUInt32 uiSeed)
{
  const nsSimdVec4i result = nsSimdVec4i::Truncate(nsSimdRandom::FloatMinMax(nsSimdVec4i(inout_iPosition), nsSimdVec4f((float)iMinValue), nsSimdVec4f((float)iMaxValue), nsSimdVec4u(uiSeed)));
  ++inout_iPosition;
  return result.x();
}

// static
float nsScriptExtensionClass_StableRandom::FloatZeroToOne(int& inout_iPosition, nsUInt32 uiSeed)
{
  const nsSimdVec4f result = nsSimdRandom::FloatZeroToOne(nsSimdVec4i(inout_iPosition), nsSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
float nsScriptExtensionClass_StableRandom::FloatMinMax(int& inout_iPosition, float fMinValue, float fMaxValue, nsUInt32 uiSeed)
{
  const nsSimdVec4f result = nsSimdRandom::FloatMinMax(nsSimdVec4i(inout_iPosition), nsSimdVec4f(fMinValue), nsSimdVec4f(fMaxValue), nsSimdVec4u(uiSeed));
  ++inout_iPosition;
  return result.x();
}

// static
nsVec3 nsScriptExtensionClass_StableRandom::Vec3MinMax(int& inout_iPosition, const nsVec3& vMinValue, const nsVec3& vMaxValue, nsUInt32 uiSeed)
{
  const nsSimdVec4i offset(0, 1, 2, 3);
  const nsSimdVec4f result = nsSimdRandom::FloatMinMax(nsSimdVec4i(inout_iPosition) + offset, nsSimdConversion::ToVec3(vMinValue), nsSimdConversion::ToVec3(vMaxValue), nsSimdVec4u(uiSeed));
  inout_iPosition += 4;
  return nsSimdConversion::ToVec3(result);
}


NS_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_StableRandom);
