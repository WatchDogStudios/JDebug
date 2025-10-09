#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Recording/JvdShape.h>

NS_BEGIN_STATIC_REFLECTED_ENUM(nsJvdShapeType, 1)
  NS_ENUM_CONSTANT(nsJvdShapeType::Unknown),
  NS_ENUM_CONSTANT(nsJvdShapeType::Box),
  NS_ENUM_CONSTANT(nsJvdShapeType::Sphere),
  NS_ENUM_CONSTANT(nsJvdShapeType::Capsule),
  NS_ENUM_CONSTANT(nsJvdShapeType::Cylinder),
  NS_ENUM_CONSTANT(nsJvdShapeType::Convex),
  NS_ENUM_CONSTANT(nsJvdShapeType::TriangleMesh)
NS_END_STATIC_REFLECTED_ENUM;

NS_STATICLINK_FILE(JVDSDK, Recording_JvdShape);
