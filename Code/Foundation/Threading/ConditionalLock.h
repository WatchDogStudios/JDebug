#pragma once

/// \brief RAII lock guard that conditionally acquires and releases locks based on runtime conditions
///
/// Provides the same automatic lock management as nsLock but only performs the actual locking
/// when a boolean condition is met. Useful in scenarios where locking is only required under
/// certain circumstances, avoiding unnecessary synchronization overhead when protection is not needed.
///
/// The condition is evaluated once at construction time. If false, no locking occurs throughout
/// the object's lifetime, making this essentially a no-op with zero runtime cost.
template <typename T>
class nsConditionalLock
{
public:
  NS_ALWAYS_INLINE explicit nsConditionalLock(T& lock, bool bCondition)
    : m_lock(lock)
    , m_bCondition(bCondition)
  {
    if (m_bCondition)
    {
      m_lock.Lock();
    }
  }

  NS_ALWAYS_INLINE ~nsConditionalLock()
  {
    if (m_bCondition)
    {
      m_lock.Unlock();
    }
  }

private:
  nsConditionalLock();
  nsConditionalLock(const nsConditionalLock<T>& rhs);
  void operator=(const nsConditionalLock<T>& rhs);

  T& m_lock;
  bool m_bCondition;
};
