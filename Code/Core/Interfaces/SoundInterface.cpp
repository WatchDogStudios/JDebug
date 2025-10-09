#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>

nsResult nsSoundInterface::PlaySound(nsWorld* pWorld, nsStringView sResourceID, const nsTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (nsSoundInterface* pSoundInterface = nsSingletonRegistry::GetSingletonInstance<nsSoundInterface>())
  {
    return pSoundInterface->OneShotSound(pWorld, sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return NS_FAILURE;
}

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_Sound, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(PlaySound, In, "World", In, "Resource", In, "GlobalPosition", In, "GlobalRotation", In, "Pitch", In, "Volume", In, "BlockToLoad")->AddAttributes(
      new nsFunctionArgumentAttributes(3, new nsDefaultValueAttribute(1.0f)),
      new nsFunctionArgumentAttributes(4, new nsDefaultValueAttribute(1.0f)),
      new nsFunctionArgumentAttributes(5, new nsDefaultValueAttribute(true))
    ),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("Sound"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on

void nsScriptExtensionClass_Sound::PlaySound(nsWorld* pWorld, nsStringView sResourceID, const nsVec3& vGlobalPos, const nsQuat& qGlobalRot, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  nsSoundInterface::PlaySound(pWorld, sResourceID, nsTransform(vGlobalPos, qGlobalRot), fPitch, fVolume, bBlockIfNotLoaded).IgnoreResult();
}


NS_STATICLINK_FILE(Core, Core_Interfaces_SoundInterface);
