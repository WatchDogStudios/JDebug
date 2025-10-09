#pragma once

#include <JVDSDK/Recording/JvdRecordingTypes.h>

class NS_JVDSDK_DLL nsJvdPlaybackController
{
public:
  nsJvdPlaybackController();

  void LoadClip(const nsJvdClip& clip);
  void Reset();

  /// \brief Advances playback by the given delta time and outputs the interpolated frame.
  /// Returns false when playback has reached the end of the clip.
  bool Step(nsTime deltaTime, nsJvdFrame& outFrame);

  void SetLoop(bool bLoop) { m_bLoop = bLoop; }
  bool GetLoop() const { return m_bLoop; }

  nsTime GetPlaybackPosition() const { return m_CurrentTime; }
  void SetPlaybackPosition(nsTime time) { m_CurrentTime = time; }

private:
  const nsJvdClip* m_pClip = nullptr;
  nsTime m_CurrentTime = nsTime::MakeZero();
  bool m_bLoop = false;
};
