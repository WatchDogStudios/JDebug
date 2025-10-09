#pragma once

#include <JVDSDK/JVDSDKDLL.h>

#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec3.h>

#include <Jolt/Jolt.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

namespace nsJvdConversion
{
  NS_ALWAYS_INLINE nsVec3 ToVec3(const JPH::Vec3& value)
  {
    return nsVec3(value.GetX(), value.GetY(), value.GetZ());
  }

#if defined(JPH_DOUBLE_PRECISION)
  NS_ALWAYS_INLINE nsVec3 ToVec3(const JPH::RVec3& value)
  {
    return nsVec3(static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
  }
#endif

  NS_ALWAYS_INLINE nsQuat ToQuat(const JPH::Quat& value)
  {
    return nsQuat::MakeFromElements(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
  }

  NS_ALWAYS_INLINE nsTransform ToTransform(const JPH::RVec3& position, const JPH::Quat& rotation, const nsVec3& scale = nsVec3(1.0f))
  {
    nsTransform result;
    result.m_vPosition = ToVec3(position);
    result.m_qRotation = ToQuat(rotation);
    result.m_vScale = scale;
    return result;
  }

  NS_ALWAYS_INLINE JPH::Vec3 ToJolt(const nsVec3& value)
  {
    return JPH::Vec3(value.x, value.y, value.z);
  }

  NS_ALWAYS_INLINE JPH::Quat ToJolt(const nsQuat& value)
  {
    return JPH::Quat(value.x, value.y, value.z, value.w);
  }
} // namespace nsJvdConversion
