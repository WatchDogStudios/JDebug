#pragma once

#include <JVDSDK/JVDSDKDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>

struct NS_JVDSDK_DLL nsJvdShapeType
{
  using StorageType = nsUInt8;

  enum Enum
  {
    Unknown,
    Box,
    Sphere,
    Capsule,
    Cylinder,
    Convex,
    TriangleMesh,

    ENUM_COUNT,

    Default = Unknown,
  };
};

NS_DECLARE_REFLECTABLE_TYPE(NS_JVDSDK_DLL, nsJvdShapeType);
