#pragma once

#include <JVDSDK/JVDSDKDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Communication/Implementation/TelemetryMessage.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Utilities/Progress.h>
#include <Foundation/Utilities/Stats.h>

#include <Core/World/Declarations.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
