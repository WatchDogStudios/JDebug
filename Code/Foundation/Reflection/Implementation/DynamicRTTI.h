#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief Adds dynamic reflection capabilities to a class declaration.
///
/// This macro must be placed in the class declaration of every type that needs dynamic reflection.
/// It adds the necessary infrastructure for runtime type identification, including static RTTI
/// storage and helper typedefs. The class must derive from nsReflectedClass (directly or indirectly)
/// to provide the virtual GetDynamicRTTI() method.
///
/// Unlike NS_ADD_DYNAMIC_REFLECTION, this variant does not automatically implement GetDynamicRTTI(),
/// allowing for custom implementations or abstract base classes.
#define NS_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE) \
  NS_ALLOW_PRIVATE_PROPERTIES(SELF);                         \
                                                             \
public:                                                      \
  using OWNTYPE = SELF;                                      \
  using SUPER = BASE_TYPE;                                   \
  NS_ALWAYS_INLINE static const nsRTTI* GetStaticRTTI()      \
  {                                                          \
    return &SELF::s_RTTI;                                    \
  }                                                          \
                                                             \
private:                                                     \
  static nsRTTI s_RTTI;                                      \
  NS_REFLECTION_DEBUG_CODE


#define NS_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)      \
  NS_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)  \
public:                                                 \
  virtual const nsRTTI* GetDynamicRTTI() const override \
  {                                                     \
    return &SELF::s_RTTI;                               \
  }


#if NS_ENABLED(NS_COMPILE_FOR_DEVELOPMENT) && NS_ENABLED(NS_COMPILER_MSVC)

#  define NS_REFLECTION_DEBUG_CODE                       \
    static const nsRTTI* ReflectionDebug_GetParentType() \
    {                                                    \
      return __super::GetStaticRTTI();                   \
    }

#  define NS_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#  define NS_REFLECTION_DEBUG_CODE /*empty*/
#  define NS_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// \brief Begins the implementation block for dynamic reflection of a type.
///
/// This macro starts the definition of the static RTTI object for a type, enabling
/// runtime type information, property access, and dynamic instantiation. Must be
/// paired with NS_END_DYNAMIC_REFLECTED_TYPE in the implementation file.
///
/// \param Type
///   The type being reflected. Must have used NS_ADD_DYNAMIC_REFLECTION in its declaration.
/// \param Version
///   Version number for serialization compatibility. Increment when changing reflection data.
/// \param AllocatorType
///   Controls dynamic instantiation capability:
///   - nsRTTINoAllocator: Type cannot be instantiated dynamically (abstract/interface types)
///   - nsRTTIDefaultAllocator<Type>: Standard heap allocation for concrete types
///   - Custom allocator: Specialized allocation strategy for pool/stack allocated types
#define NS_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType) \
  NS_RTTIINFO_DECL(Type, Type::SUPER, Version)                        \
  nsRTTI Type::s_RTTI = GetRTTI((Type*)0);                            \
  NS_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, Type::SUPER, AllocatorType)

/// \brief Ends the reflection code block that was opened with NS_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define NS_END_DYNAMIC_REFLECTED_TYPE                                                                                                \
  return nsRTTI(GetTypeName((OwnType*)0), nsGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),              \
    nsVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, \
    NS_REFLECTION_DEBUG_GETPARENTFUNC);                                                                                              \
  }

/// \brief Same as NS_BEGIN_DYNAMIC_REFLECTED_TYPE but forces the type to be treated as abstract by reflection even though
/// it might not be abstract from a C++ perspective.
#define NS_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(Type, Version)     \
  NS_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, nsRTTINoAllocator) \
    flags.Add(nsTypeFlags::Abstract);

#define NS_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE NS_END_DYNAMIC_REFLECTED_TYPE

/// \brief Base class for all types that support dynamic reflection and runtime type identification.
///
/// This class provides the fundamental virtual interface for runtime type queries and
/// the foundational reflection infrastructure. All types that need dynamic reflection
/// must derive from this class, either directly or through an inheritance chain.
///
/// Key capabilities provided:
/// - Virtual GetDynamicRTTI() for runtime type identification
/// - IsInstanceOf() for type checking and inheritance queries
/// - Foundation for property access, serialization, and other reflection features
class NS_FOUNDATION_DLL nsReflectedClass : public nsNoBase
{
  NS_ADD_DYNAMIC_REFLECTION_NO_GETTER(nsReflectedClass, nsNoBase);

public:
  virtual const nsRTTI* GetDynamicRTTI() const { return &nsReflectedClass::s_RTTI; }

public:
  NS_ALWAYS_INLINE nsReflectedClass() = default;
  NS_ALWAYS_INLINE virtual ~nsReflectedClass() = default;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  bool IsInstanceOf(const nsRTTI* pType) const;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  NS_ALWAYS_INLINE bool IsInstanceOf() const
  {
    const nsRTTI* pType = nsGetStaticRTTI<T>();
    return IsInstanceOf(pType);
  }
};
