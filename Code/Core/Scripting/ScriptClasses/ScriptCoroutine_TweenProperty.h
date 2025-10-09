#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that animates a component property value over time.
///
/// Provides smooth interpolation between the current and target property values
/// using configurable easing curves. Supports any property type that can be
/// represented as a variant and interpolated.
class NS_CORE_DLL nsScriptCoroutine_TweenProperty : public nsTypedScriptCoroutine<nsScriptCoroutine_TweenProperty, nsComponentHandle, nsStringView, nsVariant, nsTime, nsEnum<nsCurveFunction>>
{
public:
  /// Initiates the property animation to the specified target value.
  void Start(nsComponentHandle hComponent, nsStringView sPropertyName, nsVariant targetValue, nsTime duration, nsEnum<nsCurveFunction> easing);
  virtual Result Update(nsTime deltaTimeSinceLastUpdate) override;

private:
  const nsAbstractMemberProperty* m_pProperty = nullptr;
  nsComponentHandle m_hComponent;
  nsVariant m_SourceValue;
  nsVariant m_TargetValue;
  nsEnum<nsCurveFunction> m_Easing;

  nsTime m_Duration;
  nsTime m_TimePassed;
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptCoroutine_TweenProperty);
