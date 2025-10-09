#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that smoothly moves a game object to a target position over time.
///
/// Provides interpolated movement with configurable easing curves for animation effects.
/// The object's position is updated each frame until the target is reached or the duration expires.
class NS_CORE_DLL nsScriptCoroutine_MoveTo : public nsTypedScriptCoroutine<nsScriptCoroutine_MoveTo, nsGameObjectHandle, nsVec3, nsTime, nsEnum<nsCurveFunction>>
{
public:
  /// Initiates the move operation to the specified target position.
  void Start(nsGameObjectHandle hObject, const nsVec3& vTargetPos, nsTime duration, nsEnum<nsCurveFunction> easing);
  virtual Result Update(nsTime deltaTimeSinceLastUpdate) override;

private:
  nsGameObjectHandle m_hObject;
  nsVec3 m_vSourcePos;
  nsVec3 m_vTargetPos;
  nsEnum<nsCurveFunction> m_Easing;

  nsTime m_Duration;
  nsTime m_TimePassed;
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptCoroutine_MoveTo);
