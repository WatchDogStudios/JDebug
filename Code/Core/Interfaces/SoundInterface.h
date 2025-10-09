#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class nsWorld;

/// \brief Interface for sound system integration providing audio playback and control functionality.
///
/// Manages sound configuration, playback, volume control, and listener positions.
/// Supports multiple listeners for split-screen gameplay and VCA group volume control.
class NS_CORE_DLL nsSoundInterface
{
public:
  /// \brief Can be called before startup to load the configs from a different file.
  /// Otherwise will automatically be loaded by the sound system startup with the default path.
  virtual void LoadConfiguration(nsStringView sFile) = 0;

  /// \brief By default the integration should auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(nsStringView sPlatform) = 0;

  /// \brief Has to be called once per frame to update all sounds
  virtual void UpdateSound() = 0;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float fVolume) = 0;
  virtual float GetMasterChannelVolume() const = 0;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) = 0;
  virtual bool GetMasterChannelMute() const = 0;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute
  /// them.
  virtual void SetMasterChannelPaused(bool bPaused) = 0;
  virtual bool GetMasterChannelPaused() const = 0;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance' or 'Speech'.
  /// Note that the FMOD strings banks are never loaded, so the given string must be a GUID (FMOD Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(nsStringView sVcaGroupGuid, float fVolume) = 0;
  virtual float GetSoundGroupVolume(nsStringView sVcaGroupGuid) const = 0;

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(nsUInt8 uiNumListeners) = 0;
  virtual nsUInt8 GetNumListeners() = 0;

  /// \brief The editor activates this to ignore the listener positions from the listener components, and instead use the editor camera as the
  /// listener position.
  virtual void SetListenerOverrideMode(bool bEnabled) = 0;

  /// \brief Sets the position for listener N. Index -1 is used for the override mode listener.
  virtual void SetListener(nsInt32 iIndex, const nsVec3& vPosition, const nsVec3& vForward, const nsVec3& vUp, const nsVec3& vVelocity) = 0;

  /// \brief Plays a sound once. Callced by nsSoundInterface::PlaySound().
  virtual nsResult OneShotSound(nsWorld* pWorld, nsStringView sResourceID, const nsTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) = 0;

  /// \brief Plays a sound once.
  ///
  /// Convenience function to call OneShotSound() without having to retrieve the nsSoundInterface first.
  ///
  /// Which sound to play is specified through a resource ID ('Asset GUID').
  /// This is not the most efficient way to load a sound, as there is no way to preload the resource.
  /// If preloading is desired, you need to access the implementation-specific resource type directly (e.g. nsFmodSoundEventResource).
  /// Also see nsFmodSoundEventResource::PlayOnce().
  /// In practice, though, sounds are typically loaded in bulk from sound-banks, and preloading is not necessary.
  ///
  /// Be aware that this does not allow to adjust volume, pitch or position after creation. Stopping is also not possible.
  /// Use a sound component, if that is necessary.
  ///
  /// Also by default a pitch of 1 is always used. If the game speed is not 1 (nsWorld clock), a custom pitch would need to be provided,
  /// if the sound should play at the same speed.
  static nsResult PlaySound(nsWorld* pWorld, nsStringView sResourceID, const nsTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};

/// \brief Script extension class providing sound functionality for scripting environments.
class NS_CORE_DLL nsScriptExtensionClass_Sound
{
public:
  static void PlaySound(nsWorld* pWorld, nsStringView sResourceID, const nsVec3& vGlobalPos, const nsQuat& qGlobalRot, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};

NS_DECLARE_REFLECTABLE_TYPE(NS_CORE_DLL, nsScriptExtensionClass_Sound);
