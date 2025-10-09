#include <JVDSDK/JVDSDKPCH.h>

#include <JVDSDK/Playback/JvdPlaybackController.h>

nsJvdPlaybackController::nsJvdPlaybackController() = default;

void nsJvdPlaybackController::LoadClip(const nsJvdClip& clip)
{
  m_pClip = &clip;
  m_CurrentTime = nsTime::MakeZero();
}

void nsJvdPlaybackController::Reset()
{
  m_CurrentTime = nsTime::MakeZero();
}

bool nsJvdPlaybackController::Step(nsTime deltaTime, nsJvdFrame& outFrame)
{
  if (m_pClip == nullptr || m_pClip->IsEmpty())
    return false;

  m_CurrentTime += deltaTime;

  const nsTime duration = m_pClip->GetDuration();
  if (duration.IsPositive() && m_CurrentTime > duration)
  {
    if (m_bLoop)
    {
      while (m_CurrentTime > duration)
      {
        m_CurrentTime -= duration;
      }
    }
    else
    {
      m_CurrentTime = duration;
    }
  }

  const nsJvdFrame* pFrame = m_pClip->FindFrameByTime(m_CurrentTime);
  if (pFrame == nullptr)
    return false;

  outFrame = *pFrame;
  outFrame.m_Timestamp = m_CurrentTime;
  return true;
}

NS_STATICLINK_FILE(JVDSDK, Playback_JvdPlaybackController);
