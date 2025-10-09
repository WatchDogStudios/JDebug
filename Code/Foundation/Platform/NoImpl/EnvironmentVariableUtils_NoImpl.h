#include <Foundation/FoundationInternal.h>
NS_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>

nsString nsEnvironmentVariableUtils::GetValueStringImpl(nsStringView sName, nsStringView sDefault)
{
  NS_IGNORE_UNUSED(sName);
  NS_IGNORE_UNUSED(sDefault);
  NS_ASSERT_NOT_IMPLEMENTED
  return "";
}

nsResult nsEnvironmentVariableUtils::SetValueStringImpl(nsStringView sName, nsStringView sValue)
{
  NS_IGNORE_UNUSED(sName);
  NS_IGNORE_UNUSED(sValue);
  NS_ASSERT_NOT_IMPLEMENTED
  return NS_FAILURE;
}

bool nsEnvironmentVariableUtils::IsVariableSetImpl(nsStringView sName)
{
  NS_IGNORE_UNUSED(sName);
  return false;
}

nsResult nsEnvironmentVariableUtils::UnsetVariableImpl(nsStringView sName)
{
  NS_IGNORE_UNUSED(sName);
  return NS_FAILURE;
}
