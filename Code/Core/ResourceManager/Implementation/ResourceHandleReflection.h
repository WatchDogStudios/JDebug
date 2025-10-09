#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/ResourceManager.h>

/// \brief Adds two member functions to a class, GetXyzFile() and SetXyzFile() with Xyz being equal to 'name', which allow to access the handle through strings.
///
/// This macro is just for convenience, so that one doesn't need to write this boilerplate code by hand for every resource handle that
/// should be exposed through the reflection system.
/// The accessors still need to be exposed to the reflection system like this:
///
/// NS_ACCESSOR_PROPERTY("XyzResource", GetXyzFile, SetXyzFile)->AddAttributes(new nsAssetBrowserAttribute("CompatibleAsset_Xyz")),
///
#define NS_ADD_RESOURCEHANDLE_ACCESSORS(name, member)                                  \
  void Set##name##File(nsStringView sFile)                                             \
  {                                                                                    \
    if (!sFile.IsEmpty())                                                              \
    {                                                                                  \
      member = nsResourceManager::LoadResource<decltype(member)::ResourceType>(sFile); \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
      member = {};                                                                     \
    }                                                                                  \
  }                                                                                    \
                                                                                       \
  nsStringView Get##name##File() const                                                 \
  {                                                                                    \
    return member.GetResourceID();                                                     \
  }

/// \brief Same as NS_ADD_RESOURCEHANDLE_ACCESSORS, but calls 'setterFunc' instead of assigning to 'member' directly.
///
/// This can be used, if the setter should do additional validation or bookkeeping.
#define NS_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(name, member, setterFunc)             \
  void Set##name##File(nsStringView sFile)                                                \
  {                                                                                       \
    if (!sFile.IsEmpty())                                                                 \
    {                                                                                     \
      setterFunc(nsResourceManager::LoadResource<decltype(member)::ResourceType>(sFile)); \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
      setterFunc({});                                                                     \
    }                                                                                     \
  }                                                                                       \
                                                                                          \
  nsStringView Get##name##File() const                                                    \
  {                                                                                       \
    return member.GetResourceID();                                                        \
  }


/// \brief [internal] Helper class to generate accessor functions for (private) resource handle members
template <typename Class, typename Type, Type Class::*Member>
struct nsResourceHandlePropertyAccessor
{
  static nsStringView GetValue(const Class* pInstance) { return ((*pInstance).*Member).GetResourceID(); }

  static void SetValue(Class* pInstance, nsStringView value)
  {
    if (!value.IsEmpty())
    {
      (*pInstance).*Member = nsResourceManager::LoadResource<typename Type::ResourceType>(value);
    }
    else
    {
      (*pInstance).*Member = {};
    }
  }

  static void* GetPropertyPointer(const Class* pInstance)
  {
    NS_IGNORE_UNUSED(pInstance);

    // No access to sub-properties
    return nullptr;
  }
};

/// \brief Similar to NS_MEMBER_PROPERTY, but makes it convenient to expose resource handle properties
#define NS_RESOURCE_MEMBER_PROPERTY(PropertyName, MemberName)                                                        \
  (new nsMemberProperty<OwnType, nsStringView>(PropertyName,                                                         \
    &nsResourceHandlePropertyAccessor<OwnType, NS_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &nsResourceHandlePropertyAccessor<OwnType, NS_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &nsResourceHandlePropertyAccessor<OwnType, NS_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// \brief [internal] An implementation of nsTypedMemberProperty that uses custom getter / setter functions to access a property.
template <typename Class, typename Type>
class nsResourceAccessorProperty : public nsTypedMemberProperty<nsStringView>
{
public:
  using RealType = nsStringView;
  using HandleType = typename nsTypeTraits<Type>::NonConstReferenceType;
  using ResourceType = typename HandleType::ResourceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  nsResourceAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : nsTypedMemberProperty<RealType>(szPropertyName)
  {
    NS_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      nsAbstractMemberProperty::m_Flags.Add(nsPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    NS_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)().GetResourceID();
  }

  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    NS_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", nsAbstractProperty::GetPropertyName());

    if (m_Setter)
    {
      if (!value.IsEmpty())
      {
        (static_cast<Class*>(pInstance)->*m_Setter)(nsResourceManager::LoadResource<ResourceType>(value));
      }
      else
      {
        (static_cast<Class*>(pInstance)->*m_Setter)({});
      }
    }
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};

/// \brief Similar to NS_RESOURCE_MEMBER_PROPERTY, but takes a getter and setter function that access the resource handle.
///
/// This can be used to control what other things should happen, if a handle gets modified.
#define NS_RESOURCE_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new nsResourceAccessorProperty<OwnType, NS_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))
